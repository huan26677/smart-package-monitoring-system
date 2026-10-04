#include "mpu6050.h"

#include <stdint.h>
#include <math.h>

#include "esp_log.h"

#define MPU6050_ADDRESS         0x68

#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C

#define MPU6050_REG_ACCEL_XOUT_H 0x3B

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
     * 1 kHz / (1 + 4)
     * =
     * 200 Hz
     */

    ret =
        mpu6050_write_register(
            MPU6050_REG_SMPLRT_DIV,
            4
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

esp_err_t mpu6050_read(
    mpu6050_data_t *data
)
{
    /*
     * Doc lien tuc 14 byte:
     *
     * AX H
     * AX L
     *
     * AY H
     * AY L
     *
     * AZ H
     * AZ L
     *
     * TEMP H
     * TEMP L
     *
     * GX H
     * GX L
     *
     * GY H
     * GY L
     *
     * GZ H
     * GZ L
     */

    uint8_t raw[14];


    esp_err_t ret =
        mpu6050_read_registers(
            MPU6050_REG_ACCEL_XOUT_H,
            raw,
            sizeof(raw)
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


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


    return ESP_OK;
}