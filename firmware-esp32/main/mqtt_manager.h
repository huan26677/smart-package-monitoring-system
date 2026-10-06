#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_err.h"

#include "mpu6050.h"
#include "impact_detector.h"


esp_err_t mqtt_manager_init(
    const char *broker_uri
);


bool mqtt_manager_is_connected(void);
int mqtt_manager_publish_motion(const char *payload,int length,bool capture);


esp_err_t mqtt_manager_publish_telemetry(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection,
    int wifi_rssi
);


esp_err_t mqtt_manager_publish_event(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection
);

esp_err_t mqtt_manager_publish_location_scan(
    const wifi_ap_record_t *records,
    uint16_t count
);

#endif
