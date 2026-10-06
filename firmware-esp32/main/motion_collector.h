#ifndef MOTION_COLLECTOR_H
#define MOTION_COLLECTOR_H
#include "esp_err.h"
#include "sensor_data.h"
#include "impact_detector.h"
#include "cJSON.h"
esp_err_t motion_collector_init(void);
void motion_collector_add(const mpu6050_data_t *sensor,const impact_result_t *result,uint64_t sample_ms);
void motion_collector_discard_partial(void);
void motion_collector_control(const cJSON *root);
void motion_collector_receipt(const cJSON *root);
#endif
