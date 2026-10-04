#include "time_manager.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG =
    "TIME";


static bool time_synced =
    false;


/* =========================================================
 * CALLBACK
 * ========================================================= */

static void time_sync_notification_cb(
    struct timeval *tv
)
{
    time_synced = true;

    ESP_LOGI(
        TAG,
        "NTP time synchronized"
    );
}


/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t time_manager_init(void)
{
    /*
     * Viet Nam = UTC+7
     */
    setenv(
        "TZ",
        "ICT-7",
        1
    );

    tzset();


    ESP_LOGI(
        TAG,
        "Khoi tao SNTP..."
    );


    esp_sntp_setoperatingmode(
        SNTP_OPMODE_POLL
    );


    esp_sntp_setservername(
        0,
        "pool.ntp.org"
    );


    esp_sntp_set_time_sync_notification_cb(
        time_sync_notification_cb
    );


    esp_sntp_init();


    return ESP_OK;
}


/* =========================================================
 * IS SYNCED
 * ========================================================= */

bool time_manager_is_synced(void)
{
    return time_synced;
}


/* =========================================================
 * UNIX TIMESTAMP
 * ========================================================= */

int64_t time_manager_get_epoch(void)
{
    if (!time_synced)
    {
        return 0;
    }


    time_t now;

    time(
        &now
    );


    return (int64_t)now;
}


/* =========================================================
 * FORMAT TIME
 * ========================================================= */

void time_manager_get_string(
    char *buffer,
    size_t buffer_size
)
{
    if (
        buffer == NULL ||
        buffer_size == 0
    )
    {
        return;
    }


    if (!time_synced)
    {
        snprintf(
            buffer,
            buffer_size,
            "TIME UNSYNCED"
        );

        return;
    }


    time_t now;

    struct tm timeinfo;


    time(
        &now
    );


    localtime_r(
        &now,
        &timeinfo
    );


    strftime(
        buffer,
        buffer_size,
        "%Y-%m-%d %H:%M:%S",
        &timeinfo
    );
}
/* =========================================================
 * FORMAT UNIX TIMESTAMP
 * ========================================================= */

void time_manager_format_epoch(
    int64_t timestamp,
    char *buffer,
    size_t buffer_size
)
{
    if (
        buffer == NULL ||
        buffer_size == 0
    )
    {
        return;
    }


    /*
     * timestamp = 0
     * nghia la event cu hoac luc chua co NTP.
     */
    if (timestamp <= 0)
    {
        snprintf(
            buffer,
            buffer_size,
            "UNKNOWN"
        );

        return;
    }


    time_t event_time =
        (time_t)timestamp;


    struct tm timeinfo;


    localtime_r(
        &event_time,
        &timeinfo
    );


    strftime(
        buffer,
        buffer_size,
        "%Y-%m-%d %H:%M:%S",
        &timeinfo
    );
}