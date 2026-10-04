#include "setup_portal.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "app_config.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define SETUP_AP_SSID       "SMART_PACKAGE_SETUP"
#define SETUP_AP_PASSWORD   "smart1234"

#define POST_BUFFER_SIZE    512


static const char *TAG =
    "SETUP_PORTAL";


static httpd_handle_t
    server = NULL;


/* =========================================================
 * URL DECODE
 * ========================================================= */

static int hex_to_int(
    char c
)
{
    if (
        c >= '0' &&
        c <= '9'
    )
    {
        return c - '0';
    }

    c =
        (char)tolower(
            (unsigned char)c
        );

    if (
        c >= 'a' &&
        c <= 'f'
    )
    {
        return c - 'a' + 10;
    }

    return -1;
}


static void url_decode(
    char *dst,
    const char *src
)
{
    while (*src)
    {
        if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else if (
            *src == '%' &&
            src[1] &&
            src[2]
        )
        {
            int high =
                hex_to_int(
                    src[1]
                );

            int low =
                hex_to_int(
                    src[2]
                );

            if (
                high >= 0 &&
                low >= 0
            )
            {
                *dst++ =
                    (char)(
                        (high << 4)
                        |
                        low
                    );

                src += 3;
            }
            else
            {
                *dst++ =
                    *src++;
            }
        }
        else
        {
            *dst++ =
                *src++;
        }
    }

    *dst = '\0';
}


/* =========================================================
 * WEB PAGE
 * ========================================================= */

static const char setup_html[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>"

    "<title>Smart Package Setup</title>"

    "<style>"
    "body{"
        "font-family:Arial,sans-serif;"
        "background:#f2f2f2;"
        "padding:20px;"
    "}"

    ".box{"
        "max-width:420px;"
        "margin:auto;"
        "background:white;"
        "padding:24px;"
        "border-radius:12px;"
        "box-shadow:0 2px 10px #aaa;"
    "}"

    "h2{"
        "text-align:center;"
    "}"

    "label{"
        "display:block;"
        "margin-top:15px;"
        "font-weight:bold;"
    "}"

    "input{"
        "width:100%;"
        "padding:10px;"
        "margin-top:5px;"
        "box-sizing:border-box;"
    "}"

    "button{"
        "width:100%;"
        "padding:12px;"
        "margin-top:20px;"
        "background:#222;"
        "color:white;"
        "border:0;"
        "border-radius:6px;"
        "font-size:16px;"
    "}"
    "</style>"

    "</head>"

    "<body>"

    "<div class='box'>"

    "<h2>SMART PACKAGE</h2>"

    "<p>"
    "Cau hinh ket noi cho thiet bi"
    "</p>"

    "<form method='POST' action='/save'>"

    "<label>Wi-Fi SSID</label>"
    "<input "
    "name='ssid' "
    "maxlength='32' "
    "required>"

    "<label>Wi-Fi Password</label>"
    "<input "
    "name='password' "
    "type='password' "
    "maxlength='64'>"

    "<label>MQTT Broker</label>"
    "<input "
    "name='broker' "
    "maxlength='128' "
    "placeholder='mqtt://192.168.1.100:1883' "
    "required>"

    "<button type='submit'>"
    "SAVE CONFIG"
    "</button>"

    "</form>"

    "</div>"

    "</body>"
    "</html>";


/* =========================================================
 * GET /
 * ========================================================= */

static esp_err_t root_get_handler(
    httpd_req_t *req
)
{
    httpd_resp_set_type(
        req,
        "text/html; charset=utf-8"
    );

    return httpd_resp_send(
        req,
        setup_html,
        HTTPD_RESP_USE_STRLEN
    );
}


/* =========================================================
 * POST /save
 * ========================================================= */

static esp_err_t save_post_handler(
    httpd_req_t *req
)
{
    if (
        req->content_len <= 0 ||
        req->content_len >= POST_BUFFER_SIZE
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid request"
        );

        return ESP_FAIL;
    }


    char body[
        POST_BUFFER_SIZE
    ];

    int received = 0;


    while (
        received <
        req->content_len
    )
    {
        int ret =
            httpd_req_recv(
                req,
                body + received,
                req->content_len - received
            );


        if (ret <= 0)
        {
            httpd_resp_send_err(
                req,
                HTTPD_500_INTERNAL_SERVER_ERROR,
                "Receive failed"
            );

            return ESP_FAIL;
        }


        received +=
            ret;
    }


    body[received] =
        '\0';


    char ssid_encoded[128] =
        {0};

    char password_encoded[192] =
        {0};

    char broker_encoded[256] =
        {0};


    if (
        httpd_query_key_value(
            body,
            "ssid",
            ssid_encoded,
            sizeof(ssid_encoded)
        )
        != ESP_OK
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "SSID missing"
        );

        return ESP_FAIL;
    }


    /*
     * Password duoc phep rong
     * de sau nay co the dung Wi-Fi open.
     */
    httpd_query_key_value(
        body,
        "password",
        password_encoded,
        sizeof(password_encoded)
    );


    if (
        httpd_query_key_value(
            body,
            "broker",
            broker_encoded,
            sizeof(broker_encoded)
        )
        != ESP_OK
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Broker missing"
        );

        return ESP_FAIL;
    }


    app_config_t new_config =
    {
        0
    };


    url_decode(
        new_config.wifi_ssid,
        ssid_encoded
    );


    url_decode(
        new_config.wifi_password,
        password_encoded
    );


    url_decode(
        new_config.mqtt_broker,
        broker_encoded
    );


    /*
     * Kiem tra lai do dai sau URL decode.
     */

    if (
        strlen(
            new_config.wifi_ssid
        )
        == 0
        ||
        strlen(
            new_config.wifi_ssid
        )
        >
        APP_CONFIG_SSID_MAX
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "SSID invalid"
        );

        return ESP_FAIL;
    }


    if (
        strlen(
            new_config.wifi_password
        )
        >
        APP_CONFIG_PASSWORD_MAX
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Password invalid"
        );

        return ESP_FAIL;
    }


    if (
        strlen(
            new_config.mqtt_broker
        )
        == 0
        ||
        strlen(
            new_config.mqtt_broker
        )
        >
        APP_CONFIG_BROKER_MAX
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Broker invalid"
        );

        return ESP_FAIL;
    }


    /*
     * Broker hien tai chi cho mqtt://
     */
    if (
        strncmp(
            new_config.mqtt_broker,
            "mqtt://",
            7
        )
        != 0
    )
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Broker must start with mqtt://"
        );

        return ESP_FAIL;
    }


    new_config.configured =
        true;


    esp_err_t ret =
        app_config_save(
            &new_config
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Luu config loi: %s",
            esp_err_to_name(ret)
        );


        httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Cannot save config"
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "Da nhan config moi"
    );


    ESP_LOGI(
        TAG,
        "SSID: %s",
        new_config.wifi_ssid
    );


    ESP_LOGI(
        TAG,
        "Broker: %s",
        new_config.mqtt_broker
    );


    /*
     * Tuyet doi khong log password.
     */


    const char response[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' "
        "content='width=device-width,initial-scale=1'>"
        "</head>"
        "<body style='font-family:Arial;text-align:center;padding:30px;'>"
        "<h2>CONFIG SAVED</h2>"
        "<p>ESP32 dang khoi dong lai...</p>"
        "<p>Ban co the roi khoi mang SMART_PACKAGE_SETUP.</p>"
        "</body>"
        "</html>";


    httpd_resp_set_type(
        req,
        "text/html"
    );


    httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN
    );


    /*
     * Cho dien thoai nhan response.
     */
    vTaskDelay(
        pdMS_TO_TICKS(1500)
    );


    esp_restart();


    return ESP_OK;
}


/* =========================================================
 * HTTP SERVER
 * ========================================================= */

static esp_err_t start_web_server(void)
{
    httpd_config_t config =
        HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;

    esp_err_t ret =
        httpd_start(
            &server,
            &config
        );


    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong start duoc HTTP server: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    static const httpd_uri_t root_uri =
    {
        .uri =
            "/",

        .method =
            HTTP_GET,

        .handler =
            root_get_handler,

        .user_ctx =
            NULL
    };


    static const httpd_uri_t save_uri =
    {
        .uri =
            "/save",

        .method =
            HTTP_POST,

        .handler =
            save_post_handler,

        .user_ctx =
            NULL
    };


    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            server,
            &root_uri
        )
    );


    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            server,
            &save_uri
        )
    );


    ESP_LOGI(
        TAG,
        "HTTP setup server OK"
    );


    return ESP_OK;
}


/* =========================================================
 * SETUP PORTAL
 * ========================================================= */

esp_err_t setup_portal_start(void)
{
    ESP_LOGW(
        TAG,
        "BAT DAU SETUP MODE"
    );


    esp_err_t ret =
        esp_netif_init();


    if (
        ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE
    )
    {
        return ret;
    }


    ret =
        esp_event_loop_create_default();


    if (
        ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE
    )
    {
        return ret;
    }


    esp_netif_t *ap_netif =
        esp_netif_create_default_wifi_ap();


    if (ap_netif == NULL)
    {
        return ESP_FAIL;
    }


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


    wifi_config_t ap_config =
    {
        0
    };


    snprintf(
        (char *)
            ap_config.ap.ssid,

        sizeof(
            ap_config.ap.ssid
        ),

        "%s",

        SETUP_AP_SSID
    );


    snprintf(
        (char *)
            ap_config.ap.password,

        sizeof(
            ap_config.ap.password
        ),

        "%s",

        SETUP_AP_PASSWORD
    );


    ap_config.ap.ssid_len =
        strlen(
            SETUP_AP_SSID
        );


    ap_config.ap.channel =
        1;


    ap_config.ap.max_connection =
        4;


    ap_config.ap.authmode =
        WIFI_AUTH_WPA2_PSK;


    ret =
        esp_wifi_set_mode(
            WIFI_MODE_AP
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        esp_wifi_set_config(
            WIFI_IF_AP,
            &ap_config
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    ret =
        esp_wifi_start();


    if (ret != ESP_OK)
    {
        return ret;
    }


    ESP_LOGW(
        TAG,
        "=============================="
    );


    ESP_LOGW(
        TAG,
        " SETUP WIFI: %s",
        SETUP_AP_SSID
    );


    ESP_LOGW(
        TAG,
        " PASSWORD: %s",
        SETUP_AP_PASSWORD
    );


    ESP_LOGW(
        TAG,
        " OPEN: http://192.168.4.1"
    );

    ESP_LOGW(
        TAG,
        "=============================="
    );


    esp_err_t web_ret =
        start_web_server();


    if (web_ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "HTTP SERVER FAILED: %s",
            esp_err_to_name(web_ret)
        );

        return web_ret;
    }


    ESP_LOGI(
        TAG,
        "HTTP server dang lang nghe port 80"
    );
    
    return ESP_OK;
}