#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"
#include "impact_detector.h"

#define EVENT_LOG_MAX_EVENTS 50


typedef struct
{
    uint32_t id;

    package_state_t type;

    impact_level_t impact_level;

    uint32_t uptime_ms;

    int64_t timestamp;

    float g_force;

    float relative_angle;

    float vibration_rms;

    uint8_t synced;

} event_record_t;


esp_err_t event_log_init(void);

esp_err_t event_log_add(
    package_state_t type,
    impact_level_t impact_level,
    float g_force,
    float relative_angle,
    float vibration_rms
);

void event_log_print_all(void);

size_t event_log_count(void);

esp_err_t event_log_clear(void);

bool event_log_get_first_unsynced(
    event_record_t *out_event
);

esp_err_t event_log_mark_synced(
    uint32_t event_id
);

size_t event_log_unsynced_count(void);

#endif