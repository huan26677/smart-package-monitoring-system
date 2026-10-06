#include "mpu6050.h"

#include <stdint.h>
#include <math.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#define MPU6050_ADDRESS         0x68

#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C

#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU_REG_FIFO_EN 0x23
#define MPU_REG_INT_ENABLE 0x38
#define MPU_REG_INT_STATUS 0x3A
#define MPU_REG_USER_CTRL 0x6A
#define MPU_REG_FIFO_COUNT_H 0x72
#define MPU_REG_FIFO_R_W 0x74
#define FIFO_PACKET_BYTES 14

#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_WHO_AM_I    0x75


/*
 * Accelerometer:
 * ±16G
 *
 * Sensitivity:
 * 2048 LSB/G
 */
#define ACCEL_SCALE 2048.0f


/*
 * Gyroscope:
 * ±500 degree/s
 *
 * Sensitivity:
 * 65.5 LSB/(degree/s)
 */
#define GYRO_SCALE 65.5f


static const char *TAG = "MPU6050";

static i2c_master_dev_handle_t mpu_device;

typedef enum
{
    MPU_TYPE_UNKNOWN = 0,
    MPU_TYPE_6050,
    MPU_TYPE_6500

} mpu_type_t;

static mpu_type_t mpu_type = MPU_TYPE_UNKNOWN;
static int64_t sample_clock_us;

/* =========================================================
 * WRITE REGISTER
 * ========================================================= */

static esp_err_t mpu6050_write_register(
    uint8_t reg,
    uint8_t value
)
{
    uint8_t buffer[2] =
    {
        reg,
        value
    };

    return i2c_master_transmit(
        mpu_device,
        buffer,
        sizeof(buffer),
        100
    );
}


/* =========================================================
 * READ REGISTER(S)
 * ========================================================= */

static esp_err_t mpu6050_read_registers(
    uint8_t reg,
    uint8_t *data,
    size_t length
)
{
    return i2c_master_transmit_receive(
        mpu_device,

        &reg,
        1,

        data,
        length,

        100
    );
}


/* =========================================================
 * INIT MPU6050
 * ========================================================= */

esp_err_t mpu6050_init(
    i2c_master_bus_handle_t bus_handle
)
{
    ESP_LOGI(TAG, "Khoi tao MPU6050...");


    /*
     * Them MPU6050 vao I2C bus
     */

    i2c_device_config_t device_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDRESS,
        .scl_speed_hz = 400000,
    };


    esp_err_t ret =
        i2c_master_bus_add_device(
            bus_handle,
            &device_config,
            &mpu_device
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong them duoc MPU6050 vao I2C bus"
        );

        return ret;
    }


    /*
     * Doc WHO_AM_I
     */

    uint8_t who_am_i = 0;

    ret =
        mpu6050_read_registers(
            MPU6050_REG_WHO_AM_I,
            &who_am_i,
            1
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong doc duoc WHO_AM_I"
        );

        return ret;
    }


    ESP_LOGI(
        TAG,
        "WHO_AM_I = 0x%02X",
        who_am_i
    );


    /*
    * Nhan dien chip
    *
    * MPU6050:
    * WHO_AM_I = 0x68
    *
    * MPU6500:
    * WHO_AM_I = 0x70
    */

    if (who_am_i == 0x68)
    {
        mpu_type = MPU_TYPE_6050;

        ESP_LOGI(
            TAG,
            "Phat hien MPU6050"
        );
    }
    else if (who_am_i == 0x70)
    {
        mpu_type = MPU_TYPE_6500;

        ESP_LOGW(
            TAG,
            "Phat hien MPU6500 / MPU6500-compatible"
        );
    }
    else
    {
        ESP_LOGE(
            TAG,
            "IMU khong duoc ho tro, WHO_AM_I = 0x%02X",
            who_am_i
        );

        return ESP_FAIL;
    }


    /*
     * Wake up MPU6050
     *
     * Sau khi cap nguon,
     * MPU6050 mac dinh sleep.
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_PWR_MGMT_1,
            0x00
        );

    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * CONFIG
     *
     * DLPF = 3
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_CONFIG,
            0x03
        );

    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Sample rate divider
     *
     * 1 kHz / (1 + 9)
     * =
     * 100 Hz
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_SMPLRT_DIV,
            9
        );

    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Gyroscope:
     *
     * FS_SEL = 1
     * ±500 degree/s
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_GYRO_CONFIG,
            0x08
        );

    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Accelerometer:
     *
     * AFS_SEL = 3
     * ±16G
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_ACCEL_CONFIG,
            0x18
        );

    if (ret != ESP_OK)
    {
        return ret;
    }


    ESP_LOGI(
        TAG,
        "IMU khoi tao thanh cong"
    );


    return ESP_OK;
}


/* =========================================================
 * READ SENSOR
 * ========================================================= */

static void decode_sample(const uint8_t raw[FIFO_PACKET_BYTES], mpu6050_data_t *data)
{
    int16_t accel_x =
        (int16_t)(
            (raw[0] << 8) |
            raw[1]
        );


    int16_t accel_y =
        (int16_t)(
            (raw[2] << 8) |
            raw[3]
        );


    int16_t accel_z =
        (int16_t)(
            (raw[4] << 8) |
            raw[5]
        );


    int16_t temp_raw =
        (int16_t)(
            (raw[6] << 8) |
            raw[7]
        );


    int16_t gyro_x =
        (int16_t)(
            (raw[8] << 8) |
            raw[9]
        );


    int16_t gyro_y =
        (int16_t)(
            (raw[10] << 8) |
            raw[11]
        );


    int16_t gyro_z =
        (int16_t)(
            (raw[12] << 8) |
            raw[13]
        );


    /*
     * Convert raw -> physical units
     */

    data->ax =
        accel_x / ACCEL_SCALE;

    data->ay =
        accel_y / ACCEL_SCALE;

    data->az =
        accel_z / ACCEL_SCALE;


    data->gx =
        gyro_x / GYRO_SCALE;

    data->gy =
        gyro_y / GYRO_SCALE;

    data->gz =
        gyro_z / GYRO_SCALE;


    if (mpu_type == MPU_TYPE_6050)
    {
        data->temperature =
            (temp_raw / 340.0f)
            + 36.53f;
    }
    else if (mpu_type == MPU_TYPE_6500)
    {
        data->temperature =
            (temp_raw / 333.87f)
            + 21.0f;
    }
    else
    {
        data->temperature = 0.0f;
    }


    /*
     * Tong vector gia toc
     */

    data->total_g =
        sqrtf(
            data->ax * data->ax +
            data->ay * data->ay +
            data->az * data->az
        );


}

esp_err_t mpu6050_read(mpu6050_data_t *data)
{
    uint8_t raw[FIFO_PACKET_BYTES];
    esp_err_t ret = mpu6050_read_registers(MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw));
    if (ret == ESP_OK) decode_sample(raw, data);
    return ret;
}

/* FIFO records actual sensor conversions while the ESP32 is busy with flash.
 * Packet order is accel XYZ, temperature, gyro XYZ (ascending register order).
 * No synthesized/interpolated axis values are added after a loss. */
esp_err_t mpu6050_fifo_start(void)
{
    sample_clock_us = 0;
    esp_err_t ret = mpu6050_write_register(MPU_REG_FIFO_EN, 0);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_register(MPU_REG_USER_CTRL, 0);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_register(MPU_REG_USER_CTRL, 0x04);
    if (ret != ESP_OK) return ret;
    vTaskDelay(1); // at least one scheduler tick, including CONFIG_FREERTOS_HZ=100
    ret = mpu6050_write_register(MPU_REG_INT_ENABLE, 0x10);
    if (ret != ESP_OK) return ret;
    uint8_t status;
    ret = mpu6050_read_registers(MPU_REG_INT_STATUS, &status, 1);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_register(MPU_REG_USER_CTRL, 0x40);
    if (ret != ESP_OK) return ret;
    return mpu6050_write_register(MPU_REG_FIFO_EN, 0xF8);
}

esp_err_t mpu6050_fifo_read(mpu6050_sample_t *samples, size_t capacity, size_t *count)
{
    if (!samples || !count || !capacity) return ESP_ERR_INVALID_ARG;
    *count = 0;
    uint8_t status, bytes[2];
    esp_err_t ret = mpu6050_read_registers(MPU_REG_INT_STATUS, &status, 1);
    if (ret != ESP_OK) return ret;
    if (status & 0x10) {ESP_LOGW(TAG,"FIFO tran, status=0x%02x",status);return ESP_ERR_INVALID_STATE;}
    int64_t count_start_us = esp_timer_get_time();
    ret = mpu6050_read_registers(MPU_REG_FIFO_COUNT_H, bytes, 2);
    if (ret != ESP_OK) return ret;
    int64_t anchor_us = esp_timer_get_time();
    if (anchor_us - count_start_us > 20000) {ESP_LOGW(TAG,"FIFO doc count cham %lld us",(long long)(anchor_us-count_start_us));return ESP_ERR_INVALID_STATE;}
    unsigned available = ((unsigned)bytes[0] << 8) | bytes[1];
    unsigned fifo_bytes = mpu_type == MPU_TYPE_6500 ? 512 : 1024;
    if (available >= fifo_bytes) {ESP_LOGW(TAG,"FIFO count=%u, capacity=%u",available,fifo_bytes);return ESP_ERR_INVALID_STATE;}
    size_t packets = available / FIFO_PACKET_BYTES;
    if (!packets) return ESP_OK;
    /* Read every complete packet in the count snapshot. Otherwise the anchor
     * would refer to newer packets left behind in the FIFO. */
    if (packets > capacity || packets > MPU6050_FIFO_MAX_SAMPLES) return ESP_ERR_INVALID_SIZE;
    static uint8_t raw[MPU6050_FIFO_MAX_SAMPLES * FIFO_PACKET_BYTES];
    ret = mpu6050_read_registers(MPU_REG_FIFO_R_W, raw, packets * FIFO_PACKET_BYTES);
    if (ret != ESP_OK) return ret; // caller resets FIFO after any uncertain read
    ret = mpu6050_read_registers(MPU_REG_INT_STATUS, &status, 1);
    if (ret != ESP_OK) return ret;
    if (status & 0x10) {ESP_LOGW(TAG,"FIFO tran, status=0x%02x",status);return ESP_ERR_INVALID_STATE;}
    /* FIFO has no timestamp register. Reconstruct nominal conversion times
     * from real packet order. Anchor two periods behind the count read to cover phase/jitter and avoid
     * future event times. Gently correct clock drift, preserving monotonic 9..11
     * ms spacing between batches rather than dropping a real extra packet.
     * Reject large clock disagreement (including a wrong ODR), not just overflow. */
    int64_t target_last_us = anchor_us - 20000;
    int64_t first_us = target_last_us - (int64_t)(packets - 1) * 10000;
    if (sample_clock_us) {
        first_us = sample_clock_us + 10000;
        int64_t drift_us = target_last_us - (first_us + (int64_t)(packets - 1) * 10000);
        if (drift_us > 30000 || drift_us < -30000) {ESP_LOGW(TAG,"FIFO lech nhip %lld us, packets=%u, count=%u",(long long)drift_us,(unsigned)packets,available);return ESP_ERR_INVALID_STATE;}
        if (drift_us > 5000) first_us += 1000;
        else if (drift_us < -5000) first_us -= 1000;
    }
    int64_t last_us = first_us + (int64_t)(packets - 1) * 10000;
    if (first_us < 0 || last_us > anchor_us) {ESP_LOGW(TAG,"FIFO thoi gian di truoc %lld us, packets=%u, count=%u",(long long)(last_us-anchor_us),(unsigned)packets,available);return ESP_ERR_INVALID_STATE;}
    for (size_t i = 0; i < packets; ++i) {
        decode_sample(raw + i * FIFO_PACKET_BYTES, &samples[i].data);
        samples[i].estimated_ms = (uint64_t)(first_us + (int64_t)i * 10000) / 1000;
    }
    sample_clock_us = last_us;
    *count = packets;
    return ESP_OK;
}
