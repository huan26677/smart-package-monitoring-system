#include "app_config.h"

#include <stdio.h>
#include <string.h>

#include "nvs.h"
#include "network_config.h"
#include "esp_log.h"


#define CONFIG_NAMESPACE "app_config"

#define KEY_WIFI_SSID     "wifi_ssid"
#define KEY_WIFI_PASS     "wifi_pass"
#define KEY_MQTT_BROKER   "mqtt_uri"
#define KEY_CONFIGURED    "configured"
#define KEY_FORCE_SETUP   "force_setup"

static const char *TAG =
    "APP_CONFIG";


static nvs_handle_t
    config_nvs_handle;


/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t app_config_init(void)
{
    esp_err_t ret =
        nvs_open(
            CONFIG_NAMESPACE,
            NVS_READWRITE,
            &config_nvs_handle
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong mo duoc NVS config: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    ESP_LOGI(
        TAG,
        "Config manager OK"
    );


    return ESP_OK;
}


/* =========================================================
 * LOAD STRING
 * ========================================================= */

static esp_err_t load_string(
    const char *key,
    char *buffer,
    size_t buffer_size
)
{
    size_t required_size =
        buffer_size;


    esp_err_t ret =
        nvs_get_str(
            config_nvs_handle,
            key,
            buffer,
            &required_size
        );


    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        buffer[0] = '\0';

        return ESP_ERR_NVS_NOT_FOUND;
    }


    return ret;
}


/* =========================================================
 * LOAD CONFIG
 * ========================================================= */

esp_err_t app_config_load(
    app_config_t *config
)
{
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    memset(
        config,
        0,
        sizeof(app_config_t)
    );


    uint8_t configured = 0;


    esp_err_t ret =
        nvs_get_u8(
            config_nvs_handle,
            KEY_CONFIGURED,
            &configured
        );


    if (
        ret == ESP_ERR_NVS_NOT_FOUND
    )
    {
        ESP_LOGW(
            TAG,
            "Chua co cau hinh"
        );

        config->configured =
            false;

        return ESP_OK;
    }


    if (ret != ESP_OK)
    {
        return ret;
    }


    config->configured =
        configured != 0;


    if (!config->configured)
    {
        return ESP_OK;
    }


    ret =
        load_string(
            KEY_WIFI_SSID,
            config->wifi_ssid,
            sizeof(
                config->wifi_ssid
            )
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        load_string(
            KEY_WIFI_PASS,
            config->wifi_password,
            sizeof(
                config->wifi_password
            )
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        load_string(
            KEY_MQTT_BROKER,
            config->mqtt_broker,
            sizeof(
                config->mqtt_broker
            )
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret = load_string("mqtt_user", config->mqtt_username, sizeof(config->mqtt_username));
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) return ret;
    ret = load_string("mqtt_pass", config->mqtt_password, sizeof(config->mqtt_password));
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) return ret;
    network_broker_t broker;
    if (network_parse_broker(config->mqtt_broker, &broker)) {
        strcpy(config->mqtt_broker, broker.uri);
        if (broker.has_credentials) {
            strcpy(config->mqtt_username, broker.username);
            strcpy(config->mqtt_password, broker.password);
        }
    } else {
        ESP_LOGW(TAG, "Dia chi MQTT chua hop le; mo trang cau hinh");
        config->configured = false;
    }
    ESP_LOGI(TAG, "Da nap config");


    ESP_LOGI(
        TAG,
        "WiFi SSID: %s",
        config->wifi_ssid
    );


    ESP_LOGI(
        TAG,
        "MQTT config da nap"
    );


    return ESP_OK;
}


/* =========================================================
 * SAVE CONFIG
 * ========================================================= */

esp_err_t app_config_save(
    const app_config_t *config
)
{
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    network_broker_t broker;
    if (!config->wifi_ssid[0] || strlen(config->wifi_ssid) > APP_CONFIG_SSID_MAX ||
        !network_wifi_password_valid(config->wifi_password) ||
        !network_parse_broker(config->mqtt_broker, &broker) ||
        broker.has_credentials || (!config->mqtt_username[0] && config->mqtt_password[0])) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;


    ret =
        nvs_set_str(
            config_nvs_handle,
            KEY_WIFI_SSID,
            config->wifi_ssid
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        nvs_set_str(
            config_nvs_handle,
            KEY_WIFI_PASS,
            config->wifi_password
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        nvs_set_str(
            config_nvs_handle,
            KEY_MQTT_BROKER,
            config->mqtt_broker
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret = nvs_set_str(config_nvs_handle, "mqtt_user", config->mqtt_username);
    if (ret != ESP_OK) return ret;
    ret = nvs_set_str(config_nvs_handle, "mqtt_pass", config->mqtt_password);
    if (ret != ESP_OK) return ret;

    ret =
        nvs_set_u8(
            config_nvs_handle,
            KEY_CONFIGURED,
            1
        );


    if (ret != ESP_OK)
    {
        return ret;
    }

    /*
    * Khi save config moi thanh cong,
    * thoat khoi Setup Mode.
    */
    ret =
        nvs_set_u8(
            config_nvs_handle,
            KEY_FORCE_SETUP,
            0
        );


    if (ret != ESP_OK)
    {
        return ret;
    }

    ret =
        nvs_commit(
            config_nvs_handle
        );


    if (ret == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "Da luu config"
        );
    }


    return ret;
}


/* =========================================================
 * CLEAR CONFIG
 * ========================================================= */

esp_err_t app_config_clear(void)
{
    esp_err_t ret =
        nvs_erase_all(
            config_nvs_handle
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        nvs_commit(
            config_nvs_handle
        );


    if (ret == ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Da xoa config"
        );
    }


    return ret;
}
/* =========================================================
 * REQUEST SETUP MODE
 * ========================================================= */

esp_err_t app_config_request_setup_mode(void)
{
    esp_err_t ret =
        nvs_set_u8(
            config_nvs_handle,
            KEY_FORCE_SETUP,
            1
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        nvs_commit(
            config_nvs_handle
        );


    if (ret == ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Da yeu cau Setup Mode"
        );
    }


    return ret;
}


/* =========================================================
 * IS SETUP REQUESTED?
 * ========================================================= */

bool app_config_is_setup_requested(void)
{
    uint8_t value = 0;


    esp_err_t ret =
        nvs_get_u8(
            config_nvs_handle,
            KEY_FORCE_SETUP,
            &value
        );


    if (
        ret == ESP_ERR_NVS_NOT_FOUND
    )
    {
        return false;
    }


    if (ret != ESP_OK)
    {
        return false;
    }


    return value != 0;
}


/* =========================================================
 * CANCEL SETUP MODE
 * ========================================================= */

esp_err_t app_config_cancel_setup_mode(void)
{
    esp_err_t ret =
        nvs_set_u8(
            config_nvs_handle,
            KEY_FORCE_SETUP,
            0
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    return nvs_commit(
        config_nvs_handle
    );
}
