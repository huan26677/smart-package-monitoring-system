#include "wifi_scanner.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"

#define WIFI_SCAN_MAX_APS        5
#define WIFI_SCAN_INTERVAL_MS    30000


static const char *TAG =
    "WIFI_SCAN";


/* =========================================================
 * PRINT BSSID
 * ========================================================= */

static void print_bssid(
    const uint8_t *bssid,
    char *buffer,
    size_t buffer_size
)
{
    if (
        bssid == NULL ||
        buffer == NULL ||
        buffer_size < 18
    )
    {
        return;
    }


    snprintf(
        buffer,
        buffer_size,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        bssid[0],
        bssid[1],
        bssid[2],
        bssid[3],
        bssid[4],
        bssid[5]
    );
}


/* =========================================================
 * SCAN TASK
 * ========================================================= */

static void wifi_scanner_task(
    void *parameter
)
{
    /*
     * Cho Wi-Fi manager co thoi gian ket noi.
     */
    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );


    while (1)
    {
        /*
         * Chi scan khi STA dang ONLINE.
         *
         * Neu dang Setup Mode thi task nay
         * khong duoc tao tu main.c.
         */
        if (
            !wifi_manager_is_connected()
        )
        {
            vTaskDelay(
                pdMS_TO_TICKS(2000)
            );

            continue;
        }


        ESP_LOGI(
            TAG,
            "Bat dau scan WiFi..."
        );


        wifi_scan_config_t scan_config =
        {
            .ssid = NULL,
            .bssid = NULL,

            /*
             * 0 = scan tat ca channel.
             */
            .channel = 0,

            /*
             * Lay ca AP an SSID.
             * BSSID van huu ich cho geolocation.
             */
            .show_hidden = true,

            .scan_type =
                WIFI_SCAN_TYPE_ACTIVE,

            .scan_time.active =
            {
                .min = 0,
                .max = 120
            }
        };


        /*
         * true = blocking scan.
         *
         * Chi block wifi_scanner_task,
         * KHONG block main sensor task.
         */
        esp_err_t ret =
            esp_wifi_scan_start(
                &scan_config,
                true
            );


        if (ret != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Scan loi: %s",
                esp_err_to_name(ret)
            );


            vTaskDelay(
                pdMS_TO_TICKS(
                    WIFI_SCAN_INTERVAL_MS
                )
            );

            continue;
        }


        uint16_t total_ap = 0;


        ret =
            esp_wifi_scan_get_ap_num(
                &total_ap
            );


        if (ret != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Khong lay duoc so AP: %s",
                esp_err_to_name(ret)
            );


            vTaskDelay(
                pdMS_TO_TICKS(
                    WIFI_SCAN_INTERVAL_MS
                )
            );

            continue;
        }


        ESP_LOGI(
            TAG,
            "Tim thay %u AP",
            total_ap
        );


        if (total_ap == 0)
        {
            /*
             * Khong co AP.
             */
            vTaskDelay(
                pdMS_TO_TICKS(
                    WIFI_SCAN_INTERVAL_MS
                )
            );

            continue;
        }


        uint16_t number =
            total_ap;


        if (
            number >
            WIFI_SCAN_MAX_APS
        )
        {
            number =
                WIFI_SCAN_MAX_APS;
        }


        wifi_ap_record_t
            ap_records[
                WIFI_SCAN_MAX_APS
            ];


        memset(
            ap_records,
            0,
            sizeof(ap_records)
        );


        ret =
            esp_wifi_scan_get_ap_records(
                &number,
                ap_records
            );


        if (ret != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Khong doc duoc AP records: %s",
                esp_err_to_name(ret)
            );


            vTaskDelay(
                pdMS_TO_TICKS(
                    WIFI_SCAN_INTERVAL_MS
                )
            );

            continue;
        }


        ESP_LOGI(
            TAG,
            "===== TOP %u WIFI =====",
            number
        );


        for (
            uint16_t i = 0;
            i < number;
            i++
        )
        {
            char bssid_text[18] =
                {0};


            print_bssid(
                ap_records[i].bssid,
                bssid_text,
                sizeof(bssid_text)
            );


            ESP_LOGI(
                TAG,
                "#%u | BSSID=%s | RSSI=%d | CH=%u | SSID=%.*s",

                i + 1,

                bssid_text,

                ap_records[i].rssi,

                ap_records[i].primary,

                32,
                (char *)
                    ap_records[i].ssid
            );
        }

        /*
        * Gui Top AP len MQTT.
        */
        if (
            mqtt_manager_is_connected()
        )
        {
            esp_err_t mqtt_ret =
                mqtt_manager_publish_location_scan(
                    ap_records,
                    number
                );


            if (mqtt_ret != ESP_OK)
            {
                ESP_LOGW(
                    TAG,
                    "Khong gui duoc location scan: %s",
                    esp_err_to_name(
                        mqtt_ret
                    )
                );
            }
        }
        else
        {
            ESP_LOGW(
                TAG,
                "MQTT offline - bo qua location scan"
            );
        }

        ESP_LOGI(
            TAG,
            "======================"
        );


        /*
         * Scan moi 30 giay.
         */
        vTaskDelay(
            pdMS_TO_TICKS(
                WIFI_SCAN_INTERVAL_MS
            )
        );
    }
}


/* =========================================================
 * START
 * ========================================================= */

esp_err_t wifi_scanner_start(void)
{
    BaseType_t result =
        xTaskCreate(
            wifi_scanner_task,
            "wifi_scanner",

            /*
             * Stack rieng.
             *
             * Khong an stack main nhu vu migration V1.
             */
            4096,

            NULL,

            4,

            NULL
        );


    if (
        result != pdPASS
    )
    {
        ESP_LOGE(
            TAG,
            "Khong tao duoc wifi scanner task"
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "WiFi scanner started"
    );


    return ESP_OK;
}