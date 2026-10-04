#include "mqtt_manager.h"
#include <stdio.h>
#include <string.h>
#include "event_log.h"
#include "time_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"


#define DEVICE_ID "esp32-001"


#define TOPIC_TELEMETRY \
    "smart-package/" DEVICE_ID "/telemetry"

#define TOPIC_EVENT \
    "smart-package/" DEVICE_ID "/event"

#define TOPIC_LOCATION_SCAN \
    "smart-package/" DEVICE_ID "/location-scan"

static const char *TAG =
    "MQTT";


static esp_mqtt_client_handle_t
    mqtt_client = NULL;


static bool mqtt_connected =
    false;

static volatile int
    pending_msg_id = -1;

static volatile uint32_t
    pending_event_id = 0;

static volatile int
    acknowledged_msg_id = -1;

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

    char payload[384];

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
            "\"timeText\":\"%s\""
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
                
            time_text
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

static void mqtt_sync_task(
    void *parameter
)
{
    event_record_t event;


    while (1)
    {
        /*
         * MQTT offline
         */
        if (!mqtt_connected)
        {
            pending_msg_id = -1;
            pending_event_id = 0;
            acknowledged_msg_id = -1;

            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );

            continue;
        }


        /*
         * Dang cho broker ACK.
         */
        if (pending_msg_id >= 0)
        {
            if (
                acknowledged_msg_id ==
                pending_msg_id
            )
            {
                uint32_t synced_id =
                    pending_event_id;


                ESP_LOGI(
                    TAG,
                    "Broker ACK EVENT #%lu",
                    (unsigned long)
                        synced_id
                );


                esp_err_t ret =
                    event_log_mark_synced(
                        synced_id
                    );


                if (ret != ESP_OK)
                {
                    ESP_LOGE(
                        TAG,
                        "Khong mark sync #%lu: %s",
                        (unsigned long)
                            synced_id,

                        esp_err_to_name(
                            ret
                        )
                    );
                }


                pending_msg_id = -1;
                pending_event_id = 0;
                acknowledged_msg_id = -1;
            }


            vTaskDelay(
                pdMS_TO_TICKS(100)
            );

            continue;
        }


        /*
         * Tim event offline cu nhat.
         */
        if (
            event_log_get_first_unsynced(
                &event
            )
        )
        {
            int msg_id =
                mqtt_publish_event_record(
                    &event
                );


            if (msg_id >= 0)
            {
                pending_msg_id =
                    msg_id;

                pending_event_id =
                    event.id;


                ESP_LOGI(
                    TAG,
                    "Sync EVENT #%lu",
                    (unsigned long)
                        event.id
                );
            }
        }
        else
        {
            /*
             * Khong con event cho sync.
             */
            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );
        }


        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }
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

            ESP_LOGI(
                TAG,
                "MQTT CONNECTED"
            );

            break;

        
        case MQTT_EVENT_PUBLISHED:

            ESP_LOGI(
                TAG,
                "PUBACK msg_id=%d",
                event->msg_id
            );


            acknowledged_msg_id =
                event->msg_id;

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


    esp_mqtt_client_config_t config =
    {
        .broker.address.uri =
            broker_uri,
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


    char payload[256];


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
            "\"rssi\":%d"
            "}",

            DEVICE_ID,

            sensor->total_g,

            detection->relative_tilt_deg,

            detection->vibration_rms,

            impact_state_to_string(
                detection->state
            ),

            wifi_rssi
        );


    if (
        len < 0 ||
        len >= sizeof(payload)
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }


    int msg_id =
        esp_mqtt_client_publish(
            mqtt_client,

            TOPIC_TELEMETRY,

            payload,

            0,

            0,

            0
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