#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#include "sensor_data.h"
#include <stddef.h>
#include <stdint.h>
#define MPU6050_FIFO_MAX_SAMPLES 73
typedef struct {mpu6050_data_t data; uint64_t estimated_ms;} mpu6050_sample_t;
esp_err_t mpu6050_fifo_start(void);
esp_err_t mpu6050_fifo_read(mpu6050_sample_t *samples, size_t capacity, size_t *count);

esp_err_t mpu6050_init(
    i2c_master_bus_handle_t bus_handle
);

esp_err_t mpu6050_read(
    mpu6050_data_t *data
);

#endif