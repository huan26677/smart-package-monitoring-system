#include "impact_detector.h"

#include <math.h>
#include <string.h>


#define RAD_TO_DEG 57.2957795f


/* =========================================================
 * CONFIG
 * ========================================================= */

/* 100Hz x 2 giây */
#define CALIBRATION_SAMPLES          200

#define FREE_FALL_THRESHOLD_G        0.35f
#define FREE_FALL_CONFIRM_SAMPLES    5

/* IMPACT LEVELS */
#define IMPACT_LIGHT_G      2.50f
#define IMPACT_MEDIUM_G     4.00f
#define IMPACT_STRONG_G     7.00f

#define IMPACT_THRESHOLD_G IMPACT_LIGHT_G

/* Góc nghiêng so với tư thế NORMAL */
#define TILT_THRESHOLD_DEG           35.0f
#define TILT_CONFIRM_SAMPLES         50

/* Gần lật ngược hoàn toàn */
#define FLIP_THRESHOLD_DEG           140.0f
#define FLIP_CONFIRM_SAMPLES         50

#define VIBRATION_WINDOW_SIZE        50
#define VIBRATION_RMS_THRESHOLD      0.12f

#define DROP_WAIT_SAMPLES            100

/* =========================================================
 * VIBRATION
 * ========================================================= */

static float vibration_buffer[
    VIBRATION_WINDOW_SIZE
];

static int vibration_index = 0;
static int vibration_count = 0;


/* =========================================================
 * EVENT COUNTERS
 * ========================================================= */

static int free_fall_counter = 0;
static int free_fall_recent = 0;

static int tilt_counter = 0;
static int flip_counter = 0;


/* =========================================================
 * BASELINE
 * ========================================================= */

static bool baseline_ready = false;

static int calibration_count = 0;

static float calibration_sum_x = 0.0f;
static float calibration_sum_y = 0.0f;
static float calibration_sum_z = 0.0f;

static float baseline_x = 0.0f;
static float baseline_y = 0.0f;
static float baseline_z = 1.0f;


/* =========================================================
 * STATE
 * ========================================================= */

static package_state_t last_state =
    PACKAGE_CALIBRATING;


/* =========================================================
 * HELPER
 * ========================================================= */

static float clamp_float(
    float value,
    float min_value,
    float max_value
)
{
    if (value < min_value)
        return min_value;

    if (value > max_value)
        return max_value;

    return value;
}


/* =========================================================
 * STATE NAME
 * ========================================================= */

const char *impact_state_to_string(
    package_state_t state
)
{
    switch (state)
    {
        case PACKAGE_CALIBRATING:
            return "CALIBRATING";

        case PACKAGE_NORMAL:
            return "NORMAL";

        case PACKAGE_VIBRATION:
            return "VIBRATION";

        case PACKAGE_TILT:
            return "TILT";

        case PACKAGE_FLIP:
            return "FLIP";

        case PACKAGE_FREE_FALL:
            return "FREE_FALL";

        case PACKAGE_IMPACT:
            return "IMPACT";

        case PACKAGE_DROP:
            return "DROP";

        default:
            return "UNKNOWN";
    }
}


/* =========================================================
 * INIT
 * ========================================================= */

void impact_detector_init(void)
{
    memset(
        vibration_buffer,
        0,
        sizeof(vibration_buffer)
    );

    vibration_index = 0;
    vibration_count = 0;

    free_fall_counter = 0;
    free_fall_recent = 0;

    tilt_counter = 0;
    flip_counter = 0;

    baseline_ready = false;

    calibration_count = 0;

    calibration_sum_x = 0.0f;
    calibration_sum_y = 0.0f;
    calibration_sum_z = 0.0f;

    baseline_x = 0.0f;
    baseline_y = 0.0f;
    baseline_z = 1.0f;

    last_state =
        PACKAGE_CALIBRATING;
}


/* =========================================================
 * VIBRATION RMS
 * ========================================================= */

static float calculate_vibration_rms(
    float total_g
)
{
    float dynamic =
        total_g - 1.0f;

    vibration_buffer[
        vibration_index
    ] = dynamic;

    vibration_index++;

    if (
        vibration_index >=
        VIBRATION_WINDOW_SIZE
    )
    {
        vibration_index = 0;
    }

    if (
        vibration_count <
        VIBRATION_WINDOW_SIZE
    )
    {
        vibration_count++;
    }

    float sum_square = 0.0f;

    for (
        int i = 0;
        i < vibration_count;
        i++
    )
    {
        sum_square +=
            vibration_buffer[i] *
            vibration_buffer[i];
    }

    if (vibration_count == 0)
    {
        return 0.0f;
    }

    return sqrtf(
        sum_square /
        vibration_count
    );
}
/* =========================================================
 * IMPACT LEVEL NAME
 * ========================================================= */
const char *impact_level_to_string(
    impact_level_t level
)
{
    switch (level)
    {
        case IMPACT_LEVEL_LIGHT:
            return "LIGHT";

        case IMPACT_LEVEL_MEDIUM:
            return "MEDIUM";

        case IMPACT_LEVEL_STRONG:
            return "STRONG";

        case IMPACT_LEVEL_NONE:
        default:
            return "NONE";
    }
}
/* =========================================================
 * IMPACT LEVEL CLASSIFY
 * ========================================================= */
static impact_level_t classify_impact(
    float total_g
)
{
    if (total_g >= IMPACT_STRONG_G)
    {
        return IMPACT_LEVEL_STRONG;
    }

    if (total_g >= IMPACT_MEDIUM_G)
    {
        return IMPACT_LEVEL_MEDIUM;
    }

    if (total_g >= IMPACT_LIGHT_G)
    {
        return IMPACT_LEVEL_LIGHT;
    }

    return IMPACT_LEVEL_NONE;
}
/* =========================================================
 * CALIBRATION
 * ========================================================= */

static void calibration_update(
    const mpu6050_data_t *sensor,
    impact_result_t *result
)
{
    /*
     * Chỉ lấy mẫu khi cảm biến gần trạng thái tĩnh.
     */

    if (
        sensor->total_g >= 0.90f &&
        sensor->total_g <= 1.10f
    )
    {
        calibration_sum_x += sensor->ax;
        calibration_sum_y += sensor->ay;
        calibration_sum_z += sensor->az;

        calibration_count++;
    }

    result->calibration_percent =
        calibration_count * 100 /
        CALIBRATION_SAMPLES;

    if (
        result->calibration_percent > 100
    )
    {
        result->calibration_percent = 100;
    }


    if (
        calibration_count >=
        CALIBRATION_SAMPLES
    )
    {
        baseline_x =
            calibration_sum_x /
            calibration_count;

        baseline_y =
            calibration_sum_y /
            calibration_count;

        baseline_z =
            calibration_sum_z /
            calibration_count;


        /*
         * Normalize baseline vector
         */

        float magnitude =
            sqrtf(
                baseline_x * baseline_x +
                baseline_y * baseline_y +
                baseline_z * baseline_z
            );


        if (magnitude > 0.001f)
        {
            baseline_x /= magnitude;
            baseline_y /= magnitude;
            baseline_z /= magnitude;

            baseline_ready = true;
        }
    }


    result->calibrated =
        baseline_ready;
}


/* =========================================================
 * RELATIVE ANGLE
 * ========================================================= */

static float calculate_relative_angle(
    const mpu6050_data_t *sensor
)
{
    float magnitude =
        sensor->total_g;


    if (magnitude < 0.05f)
    {
        return 0.0f;
    }


    float x =
        sensor->ax / magnitude;

    float y =
        sensor->ay / magnitude;

    float z =
        sensor->az / magnitude;


    /*
     * Dot product:
     *
     * current_vector DOT baseline_vector
     *
     * = cos(theta)
     */

    float dot =
        x * baseline_x +
        y * baseline_y +
        z * baseline_z;


    dot =
        clamp_float(
            dot,
            -1.0f,
            1.0f
        );


    float angle =
        acosf(dot) *
        RAD_TO_DEG;


    return angle;
}


/* =========================================================
 * UPDATE
 * ========================================================= */

void impact_detector_update(
    const mpu6050_data_t *sensor,
    impact_result_t *result
)
{
    /* -----------------------------------------------------
     * Absolute pitch / roll
     *
     * Chỉ dùng hiển thị.
     * KHÔNG dùng để xác định NORMAL nữa.
     * ----------------------------------------------------- */

    result->roll_deg =
        atan2f(
            sensor->ay,
            sensor->az
        )
        * RAD_TO_DEG;


    result->pitch_deg =
        atan2f(
            -sensor->ax,

            sqrtf(
                sensor->ay * sensor->ay +
                sensor->az * sensor->az
            )
        )
        * RAD_TO_DEG;


    result->peak_g =
        sensor->total_g;


    /* =====================================================
     * CALIBRATION
     * ===================================================== */

    if (!baseline_ready)
    {
        calibration_update(
            sensor,
            result
        );

        result->state =
            PACKAGE_CALIBRATING;

        result->relative_tilt_deg =
            0.0f;

        result->vibration_rms =
            0.0f;

        result->new_event =
            false;

        return;
    }


    result->calibrated = true;
    result->calibration_percent = 100;


    /* =====================================================
     * RELATIVE ANGLE
     * ===================================================== */

    float relative_angle =
        calculate_relative_angle(
            sensor
        );


    result->relative_tilt_deg =
        relative_angle;


    /* =====================================================
     * VIBRATION
     * ===================================================== */

    float vibration_rms =
        calculate_vibration_rms(
            sensor->total_g
        );


    result->vibration_rms =
        vibration_rms;


    /* =====================================================
     * FREE FALL
     * ===================================================== */

    if (
        sensor->total_g <
        FREE_FALL_THRESHOLD_G
    )
    {
        free_fall_counter++;
    }
    else
    {
        free_fall_counter = 0;
    }


    bool free_fall =
        free_fall_counter >=
        FREE_FALL_CONFIRM_SAMPLES;


    if (free_fall)
    {
        free_fall_recent =
            DROP_WAIT_SAMPLES;
    }


    if (free_fall_recent > 0)
    {
        free_fall_recent--;
    }


    /* =====================================================
     * IMPACT
     * ===================================================== */

    bool impact =
        sensor->total_g >=
        IMPACT_THRESHOLD_G;

    /* =====================================================
     * IMPACT LEVEL
     * ===================================================== */

    impact_level_t impact_level =
        classify_impact(
            sensor->total_g
        );

    /* =====================================================
     * ANGLE VALID
     *
     * Khi đang va đập / rơi,
     * accelerometer không còn chỉ đo gravity.
     * Vì vậy không dùng góc trong khoảng này.
     * ===================================================== */

    bool acceleration_stable =
        sensor->total_g >= 0.75f &&
        sensor->total_g <= 1.25f;


    /* =====================================================
     * FLIP
     * ===================================================== */

    bool flip_condition =
        acceleration_stable &&
        relative_angle >=
        FLIP_THRESHOLD_DEG;


    if (flip_condition)
    {
        flip_counter++;
    }
    else
    {
        flip_counter = 0;
    }


    bool flip =
        flip_counter >=
        FLIP_CONFIRM_SAMPLES;


    /* =====================================================
     * TILT
     * ===================================================== */

    bool tilt_condition =
        acceleration_stable &&
        relative_angle >=
        TILT_THRESHOLD_DEG &&
        relative_angle <
        FLIP_THRESHOLD_DEG;


    if (tilt_condition)
    {
        tilt_counter++;
    }
    else
    {
        tilt_counter = 0;
    }


    bool tilt =
        tilt_counter >=
        TILT_CONFIRM_SAMPLES;


    /* =====================================================
     * VIBRATION
     * ===================================================== */

    bool vibration =
        vibration_count >=
        VIBRATION_WINDOW_SIZE

        &&

        vibration_rms >=
        VIBRATION_RMS_THRESHOLD;


    /* =====================================================
     * STATE PRIORITY
     * ===================================================== */

    package_state_t new_state =
        PACKAGE_NORMAL;


    /*
     * FREE FALL + IMPACT
     * = DROP
     */

    if (
        impact &&
        free_fall_recent > 0
    )
    {
        new_state =
            PACKAGE_DROP;

        free_fall_recent = 0;
    }

    else if (impact)
    {
        new_state =
            PACKAGE_IMPACT;
    }

    else if (free_fall)
    {
        new_state =
            PACKAGE_FREE_FALL;
    }

    else if (flip)
    {
        new_state =
            PACKAGE_FLIP;
    }

    else if (tilt)
    {
        new_state =
            PACKAGE_TILT;
    }

    else if (vibration)
    {
        new_state =
            PACKAGE_VIBRATION;
    }

    else
    {
        new_state =
            PACKAGE_NORMAL;
    }

    /* =====================================================
     * IMPACT LEVEL
     * ===================================================== */

    if (
        new_state == PACKAGE_IMPACT ||
        new_state == PACKAGE_DROP
    )
    {
        result->impact_level =
            impact_level;
    }
    else
    {
        result->impact_level =
            IMPACT_LEVEL_NONE;
    }

    /* =====================================================
     * NEW EVENT
     * ===================================================== */

    result->new_event = false;


    if (
        new_state != last_state &&
        new_state != PACKAGE_NORMAL &&
        new_state != PACKAGE_CALIBRATING
    )
    {
        result->new_event =
            true;
    }


    result->state =
        new_state;


    last_state =
        new_state;
}