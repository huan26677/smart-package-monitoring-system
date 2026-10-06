#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include "esp_err.h"


#define APP_CONFIG_SSID_MAX       32
#define APP_CONFIG_PASSWORD_MAX   64
#define APP_CONFIG_BROKER_MAX     256
#define APP_CONFIG_MQTT_USER_MAX  64
#define APP_CONFIG_MQTT_PASS_MAX  128


typedef struct
{
    char wifi_ssid[
        APP_CONFIG_SSID_MAX + 1
    ];

    char wifi_password[
        APP_CONFIG_PASSWORD_MAX + 1
    ];

    char mqtt_broker[
        APP_CONFIG_BROKER_MAX + 1
    ];

    bool configured;
    char mqtt_username[APP_CONFIG_MQTT_USER_MAX + 1];
    char mqtt_password[APP_CONFIG_MQTT_PASS_MAX + 1];

} app_config_t;


/*
 * Khoi tao config manager.
 */
esp_err_t app_config_init(void);


/*
 * Doc config tu NVS.
 */
esp_err_t app_config_load(
    app_config_t *config
);


/*
 * Luu config vao NVS.
 */
esp_err_t app_config_save(
    const app_config_t *config
);


/*
 * Xoa config.
 */
esp_err_t app_config_clear(void);

esp_err_t app_config_request_setup_mode(void);

bool app_config_is_setup_requested(void);

esp_err_t app_config_cancel_setup_mode(void);

#endif
