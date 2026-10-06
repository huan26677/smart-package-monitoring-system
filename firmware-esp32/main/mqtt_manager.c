#include "mqtt_manager.h"
#include <stdio.h>
#include <string.h>
#include "event_log.h"
#include "motion_collector.h"
#include "time_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_timer.h"
#include <math.h>
#include <stdatomic.h>
#include "freertos/queue.h"


#define DEVICE_ID "esp32-001"


#define TOPIC_TELEMETRY \
    "smart-package/" DEVICE_ID "/telemetry"

#define TOPIC_EVENT_ACK "smart-package/" DEVICE_ID "/event-ack"
#define TOPIC_MOTION "smart-package/" DEVICE_ID "/motion-window"
#define TOPIC_MOTION_ACK "smart-package/" DEVICE_ID "/motion-ack"
#define TOPIC_MOTION_CONTROL "smart-package/" DEVICE_ID "/motion-control"

#define TOPIC_EVENT \
    "smart-package/" DEVICE_ID "/event"

#define TOPIC_LOCATION_SCAN \
    "smart-package/" DEVICE_ID "/location-scan"

static const char *TAG =
    "MQTT";


static esp_mqtt_client_handle_t
    mqtt_client = NULL;


static atomic_bool mqtt_connected =
    false;

static QueueHandle_t receipt_queue;
static char receipt_payload[512];
static int receipt_length;
static bool receipt_topic_valid;
static int receipt_kind;

/* =========================================================
 * PUBLISH EVENT RECORD
 * ========================================================= */

static int mqtt_publish_event_record(
    const event_record_t *event
)
{
    if (
        event == NULL ||
        mqtt_client == NULL ||
        !mqtt_connected
    )
    {
        return -1;
    }

    char time_text[32];


    time_manager_format_epoch(
        event->timestamp,
        time_text,
        sizeof(time_text)
    );

    char payload[512];
    char duration_text[16];
    if (event->measurement_version)
        snprintf(duration_text, sizeof(duration_text), "%lu", (unsigned long)event->duration_ms);
    else
        strcpy(duration_text, "null");

    int len =
        snprintf(
            payload,
            sizeof(payload),

            "{"
            "\"deviceId\":\"%s\","
            "\"eventId\":%lu,"
            "\"type\":\"%s\","
            "\"level\":\"%s\","
            "\"g\":%.3f,"
            "\"angle\":%.1f,"
            "\"vibration\":%.3f,"
            "\"uptimeMs\":%lu,"
            "\"timestamp\":%lld,"
            "\"timeText\":\"%s\","
            "\"durationMs\":%s,"
            "\"saturated\":%s"
            "}",

            DEVICE_ID,

            (unsigned long)
                event->id,

            impact_state_to_string(
                event->type
            ),

            impact_level_to_string(
                event->impact_level
            ),

            event->g_force,

            event->relative_angle,

            event->vibration_rms,

            (unsigned long)
                event->uptime_ms,

            (long long)
                event->timestamp,
                
            time_text,
            duration_text,
            event->measurement_version ? (event->saturated ? "true" : "false") : "null"
        );


    if (
        len <= 0 ||
        len >= sizeof(payload)
    )
    {
        return -1;
    }


    int msg_id =
        esp_mqtt_client_enqueue(
            mqtt_client,

            TOPIC_EVENT,

            payload,

            0,

            1,      /* QoS */

            0,      /* retain */

            true    /* store */
        );


    if (msg_id >= 0)
    {
        ESP_LOGI(
            TAG,
            "Queue EVENT #%lu | msg_id=%d",
            (unsigned long)event->id,
            msg_id
        );
    }


    return msg_id;
}

static void mqtt_sync_task(void *parameter) {
    event_record_t record;
    uint32_t pending_id = 0;
    int64_t sent_at = 0;
    while (1) {
        uint32_t ack_id;
        if (xQueueReceive(receipt_queue, &ack_id, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Only a receipt for the current durable record can complete it.
            if (ack_id == pending_id && event_log_mark_synced(ack_id) == ESP_OK) {
                ESP_LOGI(TAG, "Database confirmed EVENT #%lu", (unsigned long)ack_id);
                pending_id = 0;
            }
        }
        if (!mqtt_connected) { pending_id = 0; continue; }
        if (pending_id && esp_timer_get_time() - sent_at < 10000000LL) continue;
        pending_id = 0;
        if (event_log_get_first_unsynced(&record)) {
            pending_id = record.id;
            sent_at = esp_timer_get_time();
            if (mqtt_publish_event_record(&record) < 0) pending_id = 0;
        }
    }
}

static void receive_receipt(esp_mqtt_event_handle_t event) {
    if (event->current_data_offset == 0) {
        receipt_length = 0;
        receipt_topic_valid = event->topic_len == strlen(TOPIC_EVENT_ACK)
                && memcmp(event->topic, TOPIC_EVENT_ACK, event->topic_len) == 0;
        receipt_kind=receipt_topic_valid?1:0;
        if(event->topic_len==strlen(TOPIC_MOTION_ACK)&&!memcmp(event->topic,TOPIC_MOTION_ACK,event->topic_len)) receipt_kind=2;
        if(event->topic_len==strlen(TOPIC_MOTION_CONTROL)&&!memcmp(event->topic,TOPIC_MOTION_CONTROL,event->topic_len)&&!event->retain) receipt_kind=3;
        receipt_topic_valid=receipt_kind!=0;
    }
    if (!receipt_topic_valid || event->total_data_len < 0 || event->data_len < 0
            || event->total_data_len >= sizeof(receipt_payload)
            || event->data_len > event->total_data_len - receipt_length
            || event->current_data_offset != receipt_length) return;
    memcpy(receipt_payload + receipt_length, event->data, event->data_len);
    receipt_length += event->data_len;
    if (receipt_length != event->total_data_len) return;
    receipt_payload[receipt_length] = 0;
    cJSON *root = cJSON_Parse(receipt_payload);
    if (!root) return;
    if(receipt_kind==2) {motion_collector_receipt(root);cJSON_Delete(root);return;}
    if(receipt_kind==3) {motion_collector_control(root);cJSON_Delete(root);return;}
    cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
    cJSON *device = cJSON_GetObjectItemCaseSensitive(root, "deviceId");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "eventId");
    if (cJSON_IsString(status) && strcmp(status->valuestring, "SAVED") == 0
            && cJSON_IsString(device) && strcmp(device->valuestring, DEVICE_ID) == 0
            && cJSON_IsNumber(id) && id->valuedouble >= 1 && id->valuedouble <= UINT32_MAX
            && floor(id->valuedouble) == id->valuedouble) {
        uint32_t event_id = (uint32_t)id->valuedouble;
        xQueueSend(receipt_queue, &event_id, 0);
    }
    cJSON_Delete(root);
}

/* =========================================================
 * MQTT EVENT
 * ========================================================= */

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    esp_mqtt_event_handle_t event =
        event_data;


    switch (
        (esp_mqtt_event_id_t)event_id
    )
    {

        case MQTT_EVENT_CONNECTED:

            mqtt_connected = true;
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_EVENT_ACK, 1);
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_MOTION_ACK, 1);
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_MOTION_CONTROL, 1);

            ESP_LOGI(
                TAG,
                "MQTT CONNECTED"
            );

            break;

        
        case MQTT_EVENT_PUBLISHED:
            // PUBACK acknowledges the broker, not durable database storage.
            break;

        case MQTT_EVENT_DATA:
            receive_receipt(event);
            break;

        case MQTT_EVENT_DISCONNECTED:

            mqtt_connected = false;

            ESP_LOGW(
                TAG,
                "MQTT DISCONNECTED"
            );

            break;


        case MQTT_EVENT_ERROR:

            mqtt_connected = false;

            ESP_LOGE(
                TAG,
                "MQTT ERROR"
            );

            break;


        default:
            break;
    }
}


/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t mqtt_manager_init(
    const char *broker_uri
)
{
    if (
        broker_uri == NULL ||
        strlen(broker_uri) == 0
    )
    {
        return ESP_ERR_INVALID_ARG;
    }


    ESP_LOGI(
        TAG,
        "Broker: %s",
        broker_uri
    );


    receipt_queue = xQueueCreate(16, sizeof(uint32_t));
    if (!receipt_queue) return ESP_ERR_NO_MEM;

    esp_mqtt_client_config_t config =
    {
        .broker.address.uri =
            broker_uri,
        // A 200-sample motion frame must fit the output buffer. This also avoids
        // fragmented-message state leaking into subsequent small publishes in IDF 5.5.
        .buffer.size = 1024,
        .buffer.out_size = 24576,
    };


    mqtt_client =
        esp_mqtt_client_init(
            &config
        );


    if (mqtt_client == NULL)
    {
        ESP_LOGE(
            TAG,
            "Khong tao duoc MQTT client"
        );

        return ESP_FAIL;
    }


    esp_err_t ret =
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        esp_mqtt_client_start(
            mqtt_client
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong start duoc MQTT: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    BaseType_t task_result =
        xTaskCreate(
            mqtt_sync_task,
            "mqtt_sync",
            4096,
            NULL,
            5,
            NULL
        );


    if (
        task_result != pdPASS
    )
    {
        ESP_LOGE(
            TAG,
            "Khong tao duoc MQTT sync task"
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "MQTT client started"
    );
    return ESP_OK;
}


/* =========================================================
 * CONNECTED?
 * ========================================================= */

bool mqtt_manager_is_connected(void)
{
    return mqtt_connected;
}

int mqtt_manager_publish_motion(const char *payload,int length,bool capture) {
    if(!mqtt_connected || !mqtt_client || !payload || length<=0) return -1;
    // Skip live windows if the network is backed up; preserve room for durable event messages.
    if(esp_mqtt_client_get_outbox_size(mqtt_client)>24000) return -1;
    return esp_mqtt_client_enqueue(mqtt_client,TOPIC_MOTION,payload,length,capture?1:0,0,true);
}


/* =========================================================
 * TELEMETRY
 * ========================================================= */

esp_err_t mqtt_manager_publish_telemetry(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection,
    int wifi_rssi
)
{
    if (
        sensor == NULL ||
        detection == NULL
    )
    {
        return ESP_ERR_INVALID_ARG;
    }


    if (
        !mqtt_connected ||
        mqtt_client == NULL
    )
    {
        return ESP_ERR_INVALID_STATE;
    }


    char payload[384];


    int len =
        snprintf(
            payload,
            sizeof(payload),

            "{"
            "\"deviceId\":\"%s\","
            "\"g\":%.3f,"
            "\"angle\":%.1f,"
            "\"vibration\":%.3f,"
            "\"state\":\"%s\","
            "\"rssi\":%d,"
            "\"pendingEvents\":%u,"
            "\"rejectedEvents\":%u"
            "}",

            DEVICE_ID,

            sensor->total_g,

            detection->relative_tilt_deg,

            detection->vibration_rms,

            impact_state_to_string(
                detection->state
            ),

            wifi_rssi,
            (unsigned)event_log_unsynced_count(),
            (unsigned)event_log_rejected_count()
        );


    if (
        len < 0 ||
        len >= sizeof(payload)
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }


    int msg_id =
        esp_mqtt_client_enqueue(
            mqtt_client,

            TOPIC_TELEMETRY,

            payload,

            0,

            0,

            0,
            true
        );


    if (msg_id < 0)
    {
        return ESP_FAIL;
    }


    return ESP_OK;
}


/* =========================================================
 * EVENT
 * ========================================================= */

esp_err_t mqtt_manager_publish_event(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection
)
{
    if (
        sensor == NULL ||
        detection == NULL
    )
    {
        return ESP_ERR_INVALID_ARG;
    }


    if (
        !mqtt_connected ||
        mqtt_client == NULL
    )
    {
        return ESP_ERR_INVALID_STATE;
    }


    char payload[256];


    int len =
        snprintf(
            payload,
            sizeof(payload),

            "{"
            "\"deviceId\":\"%s\","
            "\"type\":\"%s\","
            "\"level\":\"%s\","
            "\"g\":%.3f,"
            "\"angle\":%.1f,"
            "\"vibration\":%.3f"
            "}",

            DEVICE_ID,

            impact_state_to_string(
                detection->state
            ),

            impact_level_to_string(
                detection->impact_level
            ),

            sensor->total_g,

            detection->relative_tilt_deg,

            detection->vibration_rms
        );


    if (
        len < 0 ||
        len >= sizeof(payload)
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }


    /*
     * QoS = 1
     *
     * Event quan trong hon telemetry.
     */
    int msg_id =
        esp_mqtt_client_publish(
            mqtt_client,

            TOPIC_EVENT,

            payload,

            0,

            1,

            0
        );


    if (msg_id < 0)
    {
        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "EVENT sent: %s",
        payload
    );


    return ESP_OK;
}
/* =========================================================
 * LOCATION SCAN
 * ========================================================= */

esp_err_t mqtt_manager_publish_location_scan(
    const wifi_ap_record_t *records,
    uint16_t count
)
{
    if (
        records == NULL ||
        count == 0
    )
    {
        return ESP_ERR_INVALID_ARG;
    }


    if (
        !mqtt_connected ||
        mqtt_client == NULL
    )
    {
        return ESP_ERR_INVALID_STATE;
    }


    /*
     * Top 5 AP chi can khoang vai tram byte.
     */
    char payload[768];


    int offset =
        snprintf(
            payload,
            sizeof(payload),

            "{"
            "\"deviceId\":\"%s\","
            "\"timestamp\":%lld,"
            "\"wifiAccessPoints\":["

            ,

            DEVICE_ID,

            (long long)
                time_manager_get_epoch()
        );


    if (
        offset < 0 ||
        offset >= sizeof(payload)
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }


    for (
        uint16_t i = 0;
        i < count;
        i++
    )
    {
        char bssid[18];


        snprintf(
            bssid,
            sizeof(bssid),

            "%02X:%02X:%02X:%02X:%02X:%02X",

            records[i].bssid[0],
            records[i].bssid[1],
            records[i].bssid[2],
            records[i].bssid[3],
            records[i].bssid[4],
            records[i].bssid[5]
        );


        int written =
            snprintf(
                payload + offset,
                sizeof(payload) - offset,

                "%s"
                "{"
                "\"bssid\":\"%s\","
                "\"rssi\":%d"
                "}",

                i > 0
                    ? ","
                    : "",

                bssid,

                records[i].rssi
            );


        if (
            written < 0 ||
            written >=
                sizeof(payload) - offset
        )
        {
            ESP_LOGE(
                TAG,
                "Location payload qua lon"
            );

            return ESP_ERR_INVALID_SIZE;
        }


        offset +=
            written;
    }


    int written =
        snprintf(
            payload + offset,
            sizeof(payload) - offset,
            "]}"
        );


    if (
        written < 0 ||
        written >=
            sizeof(payload) - offset
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }


    int msg_id =
        esp_mqtt_client_publish(
            mqtt_client,

            TOPIC_LOCATION_SCAN,

            payload,

            0,

            /*
             * QoS 0:
             * location scan duoc gui dinh ky,
             * mat 1 goi khong nghiem trong.
             */
            0,

            0
        );


    if (msg_id < 0)
    {
        ESP_LOGE(
            TAG,
            "Gui location scan that bai"
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "LOCATION SCAN sent | AP=%u",
        count
    );


    ESP_LOGI(
        TAG,
        "%s",
        payload
    );


    return ESP_OK;
}
