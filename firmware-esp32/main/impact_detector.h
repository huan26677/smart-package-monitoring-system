#ifndef IMPACT_DETECTOR_H
#define IMPACT_DETECTOR_H

#include <stdbool.h>
#include "sensor_data.h"

typedef enum
{
    PACKAGE_CALIBRATING = 0,
    PACKAGE_NORMAL,
    PACKAGE_VIBRATION,
    PACKAGE_TILT,
    PACKAGE_FLIP,
    PACKAGE_FREE_FALL,
    PACKAGE_IMPACT,
    PACKAGE_DROP

} package_state_t;

typedef enum
{
    IMPACT_LEVEL_NONE = 0,
    IMPACT_LEVEL_LIGHT,
    IMPACT_LEVEL_MEDIUM,
    IMPACT_LEVEL_STRONG

} impact_level_t;

typedef struct
{
    package_state_t state;

    /* Góc tuyệt đối, chỉ dùng tham khảo */
    float pitch_deg;
    float roll_deg;

    /* Góc so với tư thế chuẩn lúc bật máy */
    float relative_tilt_deg;

    float vibration_rms;
    float peak_g;

    impact_level_t impact_level;

    bool calibrated;
    int calibration_percent;

    bool new_event;
    package_state_t event_type;
    impact_level_t event_level;
    float event_peak_g;
    float event_angle;
    float event_vibration;
    uint32_t event_started_ms;
    uint32_t event_duration_ms;
    bool event_saturated;

} impact_result_t;

void impact_detector_init(void);

void impact_detector_update(
    const mpu6050_data_t *sensor,
    impact_result_t *result,
    uint32_t now_ms
);

const char *impact_level_to_string(
    impact_level_t level
);

const char *impact_state_to_string(
    package_state_t state
);

#endif