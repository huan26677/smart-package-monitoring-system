#include "event_log.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "time_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#ifndef EVENT_LOG_HOST_TEST
#include "freertos/queue.h"
#include "freertos/task.h"
static QueueHandle_t write_queue;
static void write_task(void *arg);
#endif

#define EVENT_STORE_MAGIC 0x504B4745
#define EVENT_STORE_VERSION 3
typedef struct
{
    uint32_t id;

    package_state_t type;

    impact_level_t impact_level;

    uint32_t uptime_ms;

    float g_force;

    float relative_angle;

    float vibration_rms;

    uint8_t synced;

} event_record_v1_t;


typedef struct
{
    uint32_t magic;

    uint32_t version;

    uint32_t next_id;

    uint32_t count;

    uint32_t head;

    event_record_v1_t events[
        50
    ];

} event_store_v1_t;

typedef struct {
    uint32_t id;
    package_state_t type;
    impact_level_t impact_level;
    uint32_t uptime_ms;
    int64_t timestamp;
    float g_force;
    float relative_angle;
    float vibration_rms;
    uint8_t synced;
} event_record_v2_t;
typedef struct {
    uint32_t magic, version, next_id, count, head;
    event_record_v2_t events[50];
} event_store_v2_t;

typedef struct {
    uint32_t magic, version, next_id, count, head;
    event_record_t events[EVENT_LOG_MAX_EVENTS];
    uint32_t rejected_count;
} event_store_t;
static event_store_t store;
static nvs_handle_t handle;
static SemaphoreHandle_t mutex;
static const char *TAG = "EVENT_LOG";

static esp_err_t save(void) {
    esp_err_t ret = nvs_set_blob(handle, "history", &store, sizeof(store));
    return ret == ESP_OK ? nvs_commit(handle) : ret;
}

static esp_err_t load_store(void) {
    mutex = xSemaphoreCreateMutex();
    if (!mutex) return ESP_ERR_NO_MEM;
    esp_err_t ret = nvs_open("event_log", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    size_t size = 0;
    ret = nvs_get_blob(handle, "history", NULL, &size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        memset(&store, 0, sizeof(store));
        store.magic = EVENT_STORE_MAGIC;
        store.version = EVENT_STORE_VERSION;
        store.next_id = 1;
        return save();
    }
    if (ret != ESP_OK) return ret;
    if (size == sizeof(store)) {
        ret = nvs_get_blob(handle, "history", &store, &size);
        if (ret != ESP_OK) return ret;
        if (store.magic != EVENT_STORE_MAGIC || store.version != EVENT_STORE_VERSION
                || store.count > EVENT_LOG_MAX_EVENTS || store.head >= EVENT_LOG_MAX_EVENTS)
            return ESP_FAIL;
        return ESP_OK;
    }
    memset(&store, 0, sizeof(store));
    store.magic = EVENT_STORE_MAGIC;
    store.version = EVENT_STORE_VERSION;
    if (size == sizeof(event_store_v2_t)) {
        static event_store_v2_t legacy;
        ret = nvs_get_blob(handle, "history", &legacy, &size);
        if (ret != ESP_OK) return ret;
        if (legacy.magic != EVENT_STORE_MAGIC || legacy.version != 2 || legacy.count > 50
                || legacy.head >= 50) return ESP_FAIL;
        store.count = legacy.count;
        store.next_id = legacy.next_id;
        for (unsigned i = 0; i < legacy.count; i++) {
            unsigned pos = legacy.count == 50 ? (legacy.head + i) % 50 : i;
            event_record_v2_t *src = &legacy.events[pos];
            event_record_t *dst = &store.events[i];
            dst->id = src->id; dst->type = src->type; dst->impact_level = src->impact_level;
            dst->uptime_ms = src->uptime_ms; dst->timestamp = src->timestamp;
            dst->g_force = src->g_force; dst->relative_angle = src->relative_angle;
            dst->vibration_rms = src->vibration_rms; dst->synced = src->synced;
        }
    } else if (size == sizeof(event_store_v1_t)) {
        static event_store_v1_t legacy;
        ret = nvs_get_blob(handle, "history", &legacy, &size);
        if (ret != ESP_OK) return ret;
        if (legacy.magic != EVENT_STORE_MAGIC || legacy.version != 1 || legacy.count > 50
                || legacy.head >= 50) return ESP_FAIL;
        store.count = legacy.count;
        store.next_id = legacy.next_id;
        for (unsigned i = 0; i < legacy.count; i++) {
            unsigned pos = legacy.count == 50 ? (legacy.head + i) % 50 : i;
            event_record_v1_t *src = &legacy.events[pos];
            event_record_t *dst = &store.events[i];
            dst->id = src->id; dst->type = src->type; dst->impact_level = src->impact_level;
            dst->uptime_ms = src->uptime_ms; dst->g_force = src->g_force;
            dst->relative_angle = src->relative_angle;
            dst->vibration_rms = src->vibration_rms; dst->synced = src->synced;
        }
    } else {
        ESP_LOGE(TAG, "Unrecognized history format; preserving existing NVS data");
        return ESP_FAIL;
    }
    store.head = store.count % EVENT_LOG_MAX_EVENTS;
    ESP_LOGI(TAG, "Migrated %lu events to V3", (unsigned long)store.count);
    return save();
}

static esp_err_t persist_event(const event_record_t *record) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    unsigned slot = EVENT_LOG_MAX_EVENTS;
    uint32_t oldest = UINT32_MAX;
    for (unsigned i = 0; i < EVENT_LOG_MAX_EVENTS; i++) {
        if (store.events[i].id == 0) { slot = i; break; }
        if (store.events[i].synced && store.events[i].id < oldest) {
            oldest = store.events[i].id; slot = i;
        }
    }
    if (slot == EVENT_LOG_MAX_EVENTS || store.next_id == 0) {
        store.rejected_count++;
        save();
        xSemaphoreGive(mutex);
        ESP_LOGE(TAG, "EVENT BUFFER FULL: preserving unsynced records; new event rejected");
        return ESP_ERR_NO_MEM;
    }
    event_record_t previous = store.events[slot];
    event_record_t *event = &store.events[slot];
    memset(event, 0, sizeof(*event));
    event->id = store.next_id++;
    uint32_t id = event->id;
    *event = *record;
    event->id = id;
    if (!previous.id) store.count++;
    esp_err_t ret = save();
    if (ret != ESP_OK) {
        *event = previous;
        store.next_id--;
        if (!previous.id) store.count--;
    }
    xSemaphoreGive(mutex);
    return ret;
}

esp_err_t event_log_init(void) {
    esp_err_t ret = load_store();
#ifndef EVENT_LOG_HOST_TEST
    if (ret == ESP_OK) {
        write_queue = xQueueCreate(32, sizeof(event_record_t));
        if (!write_queue || xTaskCreate(write_task, "event_writer", 4096, NULL, 1, NULL) != pdPASS)
            return ESP_ERR_NO_MEM;
    }
#endif
    return ret;
}

esp_err_t event_log_add(package_state_t type, impact_level_t level, float g,
        float angle, float vibration, uint32_t started_ms, uint32_t duration_ms, bool saturated) {
    event_record_t record = {0};
    record.type = type; record.impact_level = level; record.g_force = g;
    record.relative_angle = angle; record.vibration_rms = vibration;
    record.uptime_ms = started_ms; record.duration_ms = duration_ms; record.saturated = saturated;
    record.measurement_version = 1;
    int64_t epoch = time_manager_get_epoch();
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    record.timestamp = epoch > 0 ? epoch - (now_ms - started_ms) / 1000 : 0;
#ifdef EVENT_LOG_HOST_TEST
    return persist_event(&record);
#else
    if (xQueueSend(write_queue, &record, 0) == pdTRUE) return ESP_OK;
    xSemaphoreTake(mutex, portMAX_DELAY);
    store.rejected_count++;
    xSemaphoreGive(mutex);
    ESP_LOGE(TAG, "Event write queue full; new record rejected");
    return ESP_ERR_NO_MEM;
#endif
}
#ifndef EVENT_LOG_HOST_TEST
static void write_task(void *arg) {
    (void)arg;
    event_record_t record;
    while (1) {
        if (xQueueReceive(write_queue, &record, portMAX_DELAY) == pdTRUE) {
            esp_err_t ret = persist_event(&record);
            // Retry transient storage errors while preserving this queued record.
            while (ret != ESP_OK && ret != ESP_ERR_NO_MEM) {
                ESP_LOGE(TAG, "Event flash write failed; retrying");
                vTaskDelay(pdMS_TO_TICKS(1000));
                ret = persist_event(&record);
            }
        }
    }
}
#endif

size_t event_log_count(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    size_t count = store.count;
    xSemaphoreGive(mutex);
    return count;
}
size_t event_log_rejected_count(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    size_t count = store.rejected_count;
    xSemaphoreGive(mutex);
    return count;
}
size_t event_log_unsynced_count(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    size_t count = 0;
    for (unsigned i = 0; i < EVENT_LOG_MAX_EVENTS; i++)
        if (store.events[i].id && !store.events[i].synced) count++;
    xSemaphoreGive(mutex);
    return count;
}
bool event_log_get_first_unsynced(event_record_t *out) {
    if (!out) return false;
    xSemaphoreTake(mutex, portMAX_DELAY);
    unsigned slot = EVENT_LOG_MAX_EVENTS;
    uint32_t oldest = UINT32_MAX;
    for (unsigned i = 0; i < EVENT_LOG_MAX_EVENTS; i++) {
        if (store.events[i].id && !store.events[i].synced && store.events[i].id < oldest) {
            oldest = store.events[i].id; slot = i;
        }
    }
    if (slot < EVENT_LOG_MAX_EVENTS) *out = store.events[slot];
    xSemaphoreGive(mutex);
    return slot < EVENT_LOG_MAX_EVENTS;
}
esp_err_t event_log_mark_synced(uint32_t id) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    for (unsigned i = 0; i < EVENT_LOG_MAX_EVENTS; i++) {
        if (store.events[i].id == id && id) {
            if (store.events[i].synced) { ret = ESP_OK; break; }
            store.events[i].synced = 1;
            ret = save();
            if (ret != ESP_OK) store.events[i].synced = 0;
            break;
        }
    }
    xSemaphoreGive(mutex);
    return ret;
}
void event_log_print_all(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    for (unsigned i = 0; i < EVENT_LOG_MAX_EVENTS; i++) {
        event_record_t *e = &store.events[i];
        if (e->id) printf("#%lu %s peak=%.2f duration=%lu ms synced=%u\n",
                (unsigned long)e->id, impact_state_to_string(e->type), e->g_force,
                (unsigned long)e->duration_ms, e->synced);
    }
    xSemaphoreGive(mutex);
}
esp_err_t event_log_clear(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    static event_store_t previous;
    previous = store;
    // Never reuse event IDs already present in the server database.
    uint32_t next_id = store.next_id;
    memset(store.events, 0, sizeof(store.events));
    store.count = 0; store.head = 0; store.next_id = next_id;
    esp_err_t ret = save();
    if (ret != ESP_OK) store = previous;
    xSemaphoreGive(mutex);
    return ret;
}
