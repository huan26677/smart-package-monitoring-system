#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"
#include "impact_detector.h"

#define EVENT_LOG_MAX_EVENTS 128


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
    uint32_t duration_ms;
    uint8_t saturated;
    uint8_t measurement_version;

} event_record_t;


esp_err_t event_log_init(void);

esp_err_t event_log_add(
    package_state_t type,
    impact_level_t impact_level,
    float g_force,
    float relative_angle,
    float vibration_rms,
    uint32_t started_ms,
    uint32_t duration_ms,
    bool saturated
);

void event_log_print_all(void);

size_t event_log_count(void);
size_t event_log_rejected_count(void);
uint32_t event_log_next_id(void);
/* USB maintenance after a database restore: advance only, retain all records. */
esp_err_t event_log_advance_next_id(uint32_t minimum);

esp_err_t event_log_clear(void);

bool event_log_get_first_unsynced(
    event_record_t *out_event
);

esp_err_t event_log_mark_synced(
    uint32_t event_id
);

size_t event_log_unsynced_count(void);

#endif
