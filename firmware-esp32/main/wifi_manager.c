#include "wifi_manager.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"


/*
 * ============================
 * SUA WIFI CUA BAN O DAY
 * ============================
 */

static const char *TAG =
    "WIFI";


static bool wifi_connected =
    false;


/* =========================================================
 * EVENT HANDLER
 * ========================================================= */

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    /*
     * Wi-Fi driver da khoi dong.
     */
    if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START
    )
    {
        ESP_LOGI(
            TAG,
            "WiFi started. Connecting..."
        );

        esp_wifi_connect();
    }


    /*
     * Bi mat Wi-Fi.
     */
    else if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED
    )
    {
        wifi_connected = false;

        ESP_LOGW(
            TAG,
            "WiFi disconnected"
        );

        ESP_LOGI(
            TAG,
            "Trying to reconnect..."
        );

        /*
         * Tu dong ket noi lai.
         */
        esp_wifi_connect();
    }


    /*
     * Da nhan IP tu router.
     */
    else if (
        event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP
    )
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;


        wifi_connected = true;


        ESP_LOGI(
            TAG,
            "WiFi CONNECTED"
        );


        ESP_LOGI(
            TAG,
            "IP: " IPSTR,
            IP2STR(
                &event->ip_info.ip
            )
        );


        ESP_LOGI(
            TAG,
            "Gateway: " IPSTR,
            IP2STR(
                &event->ip_info.gw
            )
        );
    }
}


/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t wifi_manager_init(
    const char *ssid,
    const char *password
)
{

    if (
        ssid == NULL ||
        password == NULL ||
        strlen(ssid) == 0
    )
    {
        ESP_LOGE(
            TAG,
            "WiFi config invalid"
        );

        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(
        TAG,
        "Khoi tao WiFi..."
    );


    /*
     * Khoi tao TCP/IP stack.
     */
    esp_err_t ret =
        esp_netif_init();


    if (
        ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE
    )
    {
        return ret;
    }


    /*
     * Default event loop.
     */
    ret =
        esp_event_loop_create_default();


    if (
        ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE
    )
    {
        return ret;
    }


    /*
     * Tao network interface Station.
     */
    esp_netif_create_default_wifi_sta();


    /*
     * Khoi tao Wi-Fi driver.
     */
    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();


    ret =
        esp_wifi_init(
            &cfg
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Dang ky event Wi-Fi.
     */
    ret =
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Dang ky event IP.
     */
    ret =
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Cau hinh Station.
     */
    wifi_config_t wifi_config =
    {
        .sta =
        {
            .threshold.authmode =
                WIFI_AUTH_WPA2_PSK,

            .sae_pwe_h2e =
                WPA3_SAE_PWE_BOTH,
        }
    };


    /*
     * Copy SSID / password
     */
    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(
            wifi_config.sta.ssid
        ) - 1
    );


    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(
            wifi_config.sta.password
        ) - 1
    );


    /*
     * Station mode.
     */
    ret =
        esp_wifi_set_mode(
            WIFI_MODE_STA
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    /*
     * Bat Wi-Fi.
     */
    ret =
        esp_wifi_start();


    if (ret != ESP_OK)
    {
        return ret;
    }


    ESP_LOGI(
        TAG,
        "WiFi init OK"
    );


    return ESP_OK;
}


/* =========================================================
 * CONNECTED?
 * ========================================================= */

bool wifi_manager_is_connected(void)
{
    return wifi_connected;
}


/* =========================================================
 * RSSI
 * ========================================================= */

int8_t wifi_manager_get_rssi(void)
{
    if (!wifi_connected)
    {
        return -127;
    }


    wifi_ap_record_t ap_info;


    if (
        esp_wifi_sta_get_ap_info(
            &ap_info
        )
        != ESP_OK
    )
    {
        return -127;
    }


    return ap_info.rssi;
}