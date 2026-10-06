#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H
#include <stdint.h>
typedef struct
{
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float temperature;

    float total_g;

} mpu6050_data_t;

#endif
