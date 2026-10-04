#include "event_log.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "time_manager.h"

#define EVENT_STORE_MAGIC      0x504B4745
#define EVENT_STORE_VERSION    2

#define NVS_NAMESPACE          "event_log"
#define NVS_KEY                "history"

/*
 * =========================================================
 * OLD EVENT STORE V1
 *
 * Dung de migrate du lieu cu.
 * KHONG sua struct nay.
 * =========================================================
 */

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
        EVENT_LOG_MAX_EVENTS
    ];

} event_store_v1_t;

static const char *TAG =
    "EVENT_LOG";


typedef struct
{
    uint32_t magic;

    uint32_t version;

    uint32_t next_id;

    uint32_t count;

    uint32_t head;

    event_record_t events[
        EVENT_LOG_MAX_EVENTS
    ];

} event_store_t;


static event_store_t store;

static nvs_handle_t event_nvs_handle;


/* =========================================================
 * SAVE
 * ========================================================= */

static esp_err_t event_log_save(void)
{
    esp_err_t ret =
        nvs_set_blob(
            event_nvs_handle,
            NVS_KEY,
            &store,
            sizeof(store)
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Loi nvs_set_blob: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    ret =
        nvs_commit(
            event_nvs_handle
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Loi nvs_commit: %s",
            esp_err_to_name(ret)
        );
    }


    return ret;
}


/* =========================================================
 * RESET STORE
 * ========================================================= */

static void event_log_reset_store(void)
{
    memset(
        &store,
        0,
        sizeof(store)
    );


    store.magic =
        EVENT_STORE_MAGIC;

    store.version =
        EVENT_STORE_VERSION;

    store.next_id = 1;

    store.count = 0;

    store.head = 0;
}

/* =========================================================
 * MIGRATE V1 -> V2
 * ========================================================= */

static esp_err_t event_log_migrate_v1(
    const event_store_v1_t *old_store
)
{
    if (old_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    ESP_LOGW(
        TAG,
        "Migrating EVENT STORE V1 -> V2"
    );


    memset(
        &store,
        0,
        sizeof(store)
    );


    store.magic =
        EVENT_STORE_MAGIC;

    store.version =
        EVENT_STORE_VERSION;

    store.next_id =
        old_store->next_id;

    store.count =
        old_store->count;

    store.head =
        old_store->head;


    if (
        store.count >
        EVENT_LOG_MAX_EVENTS
    )
    {
        store.count =
            EVENT_LOG_MAX_EVENTS;
    }


    if (
        store.head >=
        EVENT_LOG_MAX_EVENTS
    )
    {
        store.head = 0;
    }


    for (
        uint32_t i = 0;
        i < EVENT_LOG_MAX_EVENTS;
        i++
    )
    {
        store.events[i].id =
            old_store->events[i].id;

        store.events[i].type =
            old_store->events[i].type;

        store.events[i].impact_level =
            old_store->events[i].impact_level;

        store.events[i].uptime_ms =
            old_store->events[i].uptime_ms;


        /*
         * Event cu khong co ngay gio that.
         *
         * Khong du thong tin de suy ra timestamp,
         * nen danh dau 0.
         */
        store.events[i].timestamp =
            0;


        store.events[i].g_force =
            old_store->events[i].g_force;

        store.events[i].relative_angle =
            old_store->events[i].relative_angle;

        store.events[i].vibration_rms =
            old_store->events[i].vibration_rms;

        store.events[i].synced =
            old_store->events[i].synced;
    }


    ESP_LOGI(
        TAG,
        "Migration thanh cong: %lu event",
        (unsigned long)store.count
    );


    /*
     * Ghi lai blob moi theo format V2.
     */
    return event_log_save();
}

/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t event_log_init(void)
{
    esp_err_t ret =
        nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &event_nvs_handle
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong mo duoc NVS: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /*
     * Buoc 1:
     * Chi hoi NVS blob dang co kich thuoc bao nhieu.
     */
    size_t stored_size = 0;


    ret =
        nvs_get_blob(
            event_nvs_handle,
            NVS_KEY,
            NULL,
            &stored_size
        );


    /*
     * Chua tung co event history.
     */
    if (
        ret ==
        ESP_ERR_NVS_NOT_FOUND
    )
    {
        ESP_LOGI(
            TAG,
            "Chua co lich su. Tao moi V2."
        );


        event_log_reset_store();


        return event_log_save();
    }


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong doc duoc kich thuoc history: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    ESP_LOGI(
        TAG,
        "Event blob size: %u bytes",
        (unsigned int)stored_size
    );


    /*
     * =====================================================
     * FORMAT V2
     * =====================================================
     */

    if (
        stored_size ==
        sizeof(event_store_t)
    )
    {
        size_t required_size =
            sizeof(store);


        ret =
            nvs_get_blob(
                event_nvs_handle,
                NVS_KEY,
                &store,
                &required_size
            );


        if (ret != ESP_OK)
        {
            return ret;
        }


        if (
            store.magic !=
                EVENT_STORE_MAGIC
            ||
            store.version !=
                EVENT_STORE_VERSION
        )
        {
            ESP_LOGW(
                TAG,
                "V2 blob khong hop le. Reset."
            );


            event_log_reset_store();


            return event_log_save();
        }


        ESP_LOGI(
            TAG,
            "Da nap %lu event V2 tu Flash",
            (unsigned long)store.count
        );


        return ESP_OK;
    }


    /*
     * =====================================================
     * FORMAT V1
     * =====================================================
     */

    if (
        stored_size ==
        sizeof(event_store_v1_t)
    )
    {
        static event_store_v1_t
            old_store;


        memset(
            &old_store,
            0,
            sizeof(old_store)
        );


        size_t required_size =
            sizeof(old_store);


        ret =
            nvs_get_blob(
                event_nvs_handle,
                NVS_KEY,
                &old_store,
                &required_size
            );


        if (ret != ESP_OK)
        {
            return ret;
        }


        if (
            old_store.magic !=
                EVENT_STORE_MAGIC
            ||
            old_store.version != 1
        )
        {
            ESP_LOGE(
                TAG,
                "V1 blob khong hop le"
            );


            return ESP_FAIL;
        }


        ESP_LOGW(
            TAG,
            "Phat hien Event Store V1"
        );


        return event_log_migrate_v1(
            &old_store
        );
    }


    /*
     * =====================================================
     * UNKNOWN FORMAT
     * =====================================================
     */

    ESP_LOGE(
        TAG,
        "Khong nhan dang duoc event blob: %u bytes",
        (unsigned int)stored_size
    );


    ESP_LOGW(
        TAG,
        "Reset event store"
    );


    event_log_reset_store();


    return event_log_save();
}

/* =========================================================
 * ADD EVENT
 * ========================================================= */

esp_err_t event_log_add(
    package_state_t type,
    impact_level_t impact_level,
    float g_force,
    float relative_angle,
    float vibration_rms
)
{
    event_record_t *event =
        &store.events[
            store.head
        ];


    memset(
        event,
        0,
        sizeof(event_record_t)
    );


    event->id =
        store.next_id++;


    event->type =
        type;


    event->impact_level =
        impact_level;


    event->uptime_ms =
        (uint32_t)(
            esp_timer_get_time() /
            1000ULL
        );

    event->timestamp =
        time_manager_get_epoch();

    event->g_force =
        g_force;


    event->relative_angle =
        relative_angle;


    event->vibration_rms =
        vibration_rms;


    /*
     * 0 = chua gui backend
     */

    event->synced = 0;


    store.head++;

    if (
        store.head >=
        EVENT_LOG_MAX_EVENTS
    )
    {
        store.head = 0;
    }


    if (
        store.count <
        EVENT_LOG_MAX_EVENTS
    )
    {
        store.count++;
    }


    ESP_LOGW(
        TAG,
        "SAVE #%lu | %s | %s | G=%.2f | Angle=%.1f",
        (unsigned long)event->id,

        impact_state_to_string(
            event->type
        ),

        impact_level_to_string(
            event->impact_level
        ),

        event->g_force,

        event->relative_angle
    );


    return event_log_save();
}


/* =========================================================
 * COUNT
 * ========================================================= */

size_t event_log_count(void)
{
    return store.count;
}


/* =========================================================
 * PRINT
 * ========================================================= */

void event_log_print_all(void)
{
    printf(
        "\n"
        "========================================\n"
        " EVENT HISTORY (%lu/%d)\n"
        "========================================\n",

        (unsigned long)store.count,

        EVENT_LOG_MAX_EVENTS
    );


    if (store.count == 0)
    {
        printf(
            "Khong co su kien nao.\n"
        );

        return;
    }


    uint32_t start_index;


    if (
        store.count <
        EVENT_LOG_MAX_EVENTS
    )
    {
        start_index = 0;
    }
    else
    {
        start_index =
            store.head;
    }


    for (
        uint32_t i = 0;
        i < store.count;
        i++
    )
    {
        uint32_t index =
            (
                start_index + i
            )
            %
            EVENT_LOG_MAX_EVENTS;


        event_record_t *event =
            &store.events[index];

        char time_text[32];

        time_manager_format_epoch(
            event->timestamp,
            time_text,
            sizeof(time_text)
        );

        printf(
            "#%lu | "
            "%s | "
            "Level:%s | "
            "G:%.2f | "
            "Angle:%.1f | "
            "Vib:%.3f | "
            "Up:%lu ms | "
            "Time:%s | "
            "Sync:%s\n",

            (unsigned long)event->id,

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

            time_text,

            event->synced
                ? "YES"
                : "NO"
        );
    }


    printf(
        "========================================\n\n"
    );
}

/* =========================================================
 * GET FIRST UNSYNCED
 * ========================================================= */

bool event_log_get_first_unsynced(
    event_record_t *out_event
)
{
    if (out_event == NULL)
    {
        return false;
    }


    if (store.count == 0)
    {
        return false;
    }


    uint32_t start_index;


    if (
        store.count <
        EVENT_LOG_MAX_EVENTS
    )
    {
        start_index = 0;
    }
    else
    {
        start_index =
            store.head;
    }


    for (
        uint32_t i = 0;
        i < store.count;
        i++
    )
    {
        uint32_t index =
            (start_index + i)
            %
            EVENT_LOG_MAX_EVENTS;


        event_record_t *event =
            &store.events[index];


        if (!event->synced)
        {
            *out_event =
                *event;

            return true;
        }
    }


    return false;
}


/* =========================================================
 * UNSYNCED COUNT
 * ========================================================= */

size_t event_log_unsynced_count(void)
{
    size_t count = 0;


    if (store.count == 0)
    {
        return 0;
    }


    uint32_t start_index;


    if (
        store.count <
        EVENT_LOG_MAX_EVENTS
    )
    {
        start_index = 0;
    }
    else
    {
        start_index =
            store.head;
    }


    for (
        uint32_t i = 0;
        i < store.count;
        i++
    )
    {
        uint32_t index =
            (start_index + i)
            %
            EVENT_LOG_MAX_EVENTS;


        if (
            !store.events[index].synced
        )
        {
            count++;
        }
    }


    return count;
}


/* =========================================================
 * MARK SYNCED
 * ========================================================= */

esp_err_t event_log_mark_synced(
    uint32_t event_id
)
{
    if (store.count == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }


    uint32_t start_index;


    if (
        store.count <
        EVENT_LOG_MAX_EVENTS
    )
    {
        start_index = 0;
    }
    else
    {
        start_index =
            store.head;
    }


    for (
        uint32_t i = 0;
        i < store.count;
        i++
    )
    {
        uint32_t index =
            (start_index + i)
            %
            EVENT_LOG_MAX_EVENTS;


        event_record_t *event =
            &store.events[index];


        if (
            event->id ==
            event_id
        )
        {
            event->synced = 1;


            ESP_LOGI(
                TAG,
                "SYNC OK #%lu",
                (unsigned long)event_id
            );


            return event_log_save();
        }
    }


    return ESP_ERR_NOT_FOUND;
}

/* =========================================================
 * CLEAR
 * ========================================================= */

esp_err_t event_log_clear(void)
{
    event_log_reset_store();

    ESP_LOGW(
        TAG,
        "Da xoa lich su su kien"
    );

    return event_log_save();
}