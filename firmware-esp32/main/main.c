#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

#include "mpu6050.h"
#include "impact_detector.h"
#include "nvs_flash.h"
#include "event_log.h"
#include "lcd1602.h"
#include "buzzer.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "app_config.h"
#include "setup_portal.h"
#include "time_manager.h"
#include "wifi_scanner.h"

#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9

static const char *TAG = "SMART_PACKAGE";


static void update_lcd_status(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection
)
{
    char line1[17] = {0};
    char line2[17] = {0};

    if (!detection->calibrated)
    {
        snprintf(line1, sizeof(line1), "CALIBRATING");
        snprintf(
            line2,
            sizeof(line2),
            "KEEP STILL %d%%",
            detection->calibration_percent
        );
    }
    else if (detection->state == PACKAGE_NORMAL)
    {
        snprintf(line1, sizeof(line1), "PACKAGE NORMAL");
        snprintf(
            line2,
            sizeof(line2),
            "ANGLE:%3.0f DEG",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_VIBRATION)
    {
        snprintf(line1, sizeof(line1), "VIBRATION");
        snprintf(
            line2,
            sizeof(line2),
            "RMS:%.2fG",
            detection->vibration_rms
        );
    }
    else if (detection->state == PACKAGE_TILT)
    {
        snprintf(line1, sizeof(line1), "TILT WARNING");
        snprintf(
            line2,
            sizeof(line2),
            "ANGLE:%3.0f DEG",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_FLIP)
    {
        snprintf(line1, sizeof(line1), "PACKAGE FLIPPED");
        snprintf(
            line2,
            sizeof(line2),
            "ANGLE:%3.0f DEG",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_FREE_FALL)
    {
        snprintf(line1, sizeof(line1), "FREE FALL!");
        snprintf(
            line2,
            sizeof(line2),
            "G:%.2f",
            sensor->total_g
        );
    }
    else if (detection->state == PACKAGE_IMPACT)
    {
        snprintf(
            line1,
            sizeof(line1),
            "IMPACT %s",
            impact_level_to_string(
                detection->impact_level
            )
        );

        snprintf(
            line2,
            sizeof(line2),
            "G:%.2f",
            sensor->total_g
        );
    }
    else if (detection->state == PACKAGE_DROP)
    {
        snprintf(line1, sizeof(line1), "PACKAGE DROPPED");
        snprintf(
            line2,
            sizeof(line2),
            "IMPACT:%.2fG",
            sensor->total_g
        );
    }
    else
    {
        snprintf(line1, sizeof(line1), "SMART PACKAGE");
        snprintf(line2, sizeof(line2), "MONITORING...");
    }

    lcd1602_print_lines(
        line1,
        line2
    );
}


static char lcd_event_line1[17] = {0};
static char lcd_event_line2[17] = {0};

static int lcd_event_hold_samples = 0;


static void lcd_capture_event(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection
)
{
    memset(
        lcd_event_line1,
        0,
        sizeof(lcd_event_line1)
    );

    memset(
        lcd_event_line2,
        0,
        sizeof(lcd_event_line2)
    );

    switch (detection->state)
    {
        case PACKAGE_IMPACT:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "IMPACT %s",
                impact_level_to_string(
                    detection->impact_level
                )
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "G:%.2f",
                sensor->total_g
            );
            break;


        case PACKAGE_DROP:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "PACKAGE DROPPED"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "IMPACT:%.2fG",
                sensor->total_g
            );
            break;


        case PACKAGE_FREE_FALL:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "FREE FALL!"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "G:%.2f",
                sensor->total_g
            );
            break;


        case PACKAGE_TILT:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "TILT WARNING"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "ANGLE:%3.0f DEG",
                detection->relative_tilt_deg
            );
            break;


        case PACKAGE_FLIP:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "PACKAGE FLIPPED"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "ANGLE:%3.0f DEG",
                detection->relative_tilt_deg
            );
            break;


        case PACKAGE_VIBRATION:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "VIBRATION"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "RMS:%.2fG",
                detection->vibration_rms
            );
            break;


        default:
            return;
    }

    /*
     * Sensor = 100 Hz
     * 200 samples = 2 giay
     */
    lcd_event_hold_samples = 200;
}


void app_main(void)
{
    /* =====================================================
     * NVS
     * ===================================================== */

    esp_err_t nvs_ret =
        nvs_flash_init();

    if (
        nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES
        ||
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND
    )
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ESP_ERROR_CHECK(
            nvs_flash_init()
        );
    }

    ESP_ERROR_CHECK(
        event_log_init()
    );

    ESP_ERROR_CHECK(
        app_config_init()
    );

    app_config_t app_config =
    {
        0
    };


    ESP_ERROR_CHECK(
        app_config_load(
            &app_config
        )
    );

    bool force_setup_mode =
        app_config_is_setup_requested();


    if (force_setup_mode)
    {
        ESP_LOGW(
            TAG,
            "FORCE SETUP MODE"
        );
    }

    /*
    * Tam thoi cho lan migrate dau tien.
    *
    * Sau khi co Setup Mode,
    * doan nay se bi xoa.
    */

    event_log_print_all();


    /* =====================================================
     * START
     * ===================================================== */

    ESP_LOGI(
        TAG,
        "=============================="
    );

    ESP_LOGI(
        TAG,
        " SMART PACKAGE MONITOR"
    );

    ESP_LOGI(
        TAG,
        "=============================="
    );


    /* =====================================================
     * LCD1602
     * ===================================================== */

    ESP_LOGI(
        TAG,
        "Khoi tao LCD1602..."
    );

    ESP_ERROR_CHECK(
        lcd1602_init()
    );

    lcd1602_print_lines(
        "SMART PACKAGE",
        "SYSTEM START"
    );

    ESP_LOGI(
        TAG,
        "LCD1602 OK"
    );

    vTaskDelay(
        pdMS_TO_TICKS(2000)
    );

    /* =====================================================
     * BUZZER
     * ===================================================== */

    ESP_ERROR_CHECK(
        buzzer_init()
    );

    ESP_LOGI(
        TAG,
        "BUZZER OK"
    );

    /* =====================================================
    * NETWORK MODE
    * ===================================================== */

    if (
        !app_config.configured ||
        force_setup_mode
    )
    {
        /*
        * Chua co Wi-Fi/MQTT config.
        *
        * ESP32 tu phat AP de cau hinh.
        */
        ESP_LOGW(
            TAG,
            "Khoi dong Setup Mode"
        );


        ESP_ERROR_CHECK(
            setup_portal_start()
        );


        /*
        * LCD bao setup mode.
        */
        lcd1602_print_lines(
            "SETUP MODE",
            "192.168.4.1"
        );
    }
    else
    {
        /*
        * Da co config.
        *
        * Ket noi Wi-Fi binh thuong.
        */
        ESP_ERROR_CHECK(
            wifi_manager_init(
                app_config.wifi_ssid,
                app_config.wifi_password
            )
        );


        ESP_LOGI(
            TAG,
            "WiFi manager started"
        );


        /*
        * Khoi dong MQTT.
        */
        ESP_ERROR_CHECK(
            mqtt_manager_init(
                app_config.mqtt_broker
            )
        );


        ESP_LOGI(
            TAG,
            "MQTT manager started"
        );

        ESP_ERROR_CHECK(
            time_manager_init()
        );

        ESP_LOGI(
            TAG,
            "Time manager started"
        );

        ESP_ERROR_CHECK(
            wifi_scanner_start()
        );


        ESP_LOGI(
            TAG,
            "WiFi scanner started"
        );
    }

    /* =====================================================
     * I2C
     * ===================================================== */

    i2c_master_bus_config_t bus_config =
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &bus_handle
        )
    );

    ESP_LOGI(
        TAG,
        "I2C OK"
    );


    /* =====================================================
     * IMU
     * ===================================================== */

    ESP_ERROR_CHECK(
        mpu6050_init(
            bus_handle
        )
    );


    /* =====================================================
     * DETECTOR
     * ===================================================== */

    impact_detector_init();

    mpu6050_data_t sensor = {0};
    impact_result_t detection = {0};

    TickType_t last_wake_time =
        xTaskGetTickCount();

    int print_counter = 0;
    int lcd_counter = 0;
    int telemetry_counter = 0;
    int wifi_offline_counter = 0;

    /* =====================================================
     * MAIN LOOP
     * ===================================================== */

    while (1)
    {
        esp_err_t ret =
            mpu6050_read(
                &sensor
            );

        if (ret == ESP_OK)
        {
            /* ---------------------------------------------
             * Xu ly du lieu
             * --------------------------------------------- */

            impact_detector_update(
                &sensor,
                &detection
            );

            telemetry_counter++;

            /* ---------------------------------------------
             * Terminal 10 Hz
             * --------------------------------------------- */

            print_counter++;

            if (print_counter >= 10)
            {
                print_counter = 0;

                if (!detection.calibrated)
                {
                    ESP_LOGI(
                        TAG,
                        "Dang lay moc NORMAL... %d%%",
                        detection.calibration_percent
                    );
                }

                printf(
                    "\n"
                    "G: %.3f | "
                    "Pitch: %.1f | "
                    "Roll: %.1f | "
                    "Relative: %.1f | "
                    "VibRMS: %.3f | "
                    "State: %s\n",

                    sensor.total_g,

                    detection.pitch_deg,
                    detection.roll_deg,

                    detection.relative_tilt_deg,

                    detection.vibration_rms,

                    impact_state_to_string(
                        detection.state
                    )
                );

                if (
                    wifi_manager_is_connected()
                )
                {
                    printf(
                        "WiFi: ONLINE | RSSI: %d dBm\n",
                        wifi_manager_get_rssi()
                    );

                    if (
                        time_manager_is_synced()
                    )
                    {
                        char time_text[32];


                        time_manager_get_string(
                            time_text,
                            sizeof(time_text)
                        );


                        printf(
                            "Time: %s\n",
                            time_text
                        );
                    }
                    else
                    {
                        printf(
                            "Time: WAITING NTP\n"
                        );
                    }

                }
                else
                {
                    printf(
                        "WiFi: OFFLINE\n"
                    );
                }
            }

            /* ---------------------------------------------
             * Neu co event moi
             * --------------------------------------------- */

            if (detection.new_event)
            {
                lcd_capture_event(
                    &sensor,
                    &detection
                );

                ESP_LOGW(
                    TAG,
                    "EVENT: %s | "
                    "LEVEL=%s | "
                    "G=%.2f | "
                    "Angle=%.1f | "
                    "VibRMS=%.3f",

                    impact_state_to_string(
                        detection.state
                    ),

                    impact_level_to_string(
                        detection.impact_level
                    ),

                    sensor.total_g,

                    detection.relative_tilt_deg,

                    detection.vibration_rms
                );

                esp_err_t save_ret =
                    event_log_add(
                        detection.state,
                        detection.impact_level,
                        sensor.total_g,
                        detection.relative_tilt_deg,
                        detection.vibration_rms
                    );

                if (
                    detection.state ==
                    PACKAGE_DROP
                )
                {
                    buzzer_alert(
                        BUZZER_ALERT_DROP
                    );
                }

                else if (
                    detection.state ==
                    PACKAGE_IMPACT
                )
                {
                    switch (
                        detection.impact_level
                    )
                    {
                        case IMPACT_LEVEL_LIGHT:

                            buzzer_alert(
                                BUZZER_ALERT_LIGHT
                            );

                            break;


                        case IMPACT_LEVEL_MEDIUM:

                            buzzer_alert(
                                BUZZER_ALERT_MEDIUM
                            );

                            break;


                        case IMPACT_LEVEL_STRONG:

                            buzzer_alert(
                                BUZZER_ALERT_STRONG
                            );

                            break;


                        default:
                            break;
                    }
                }

                if (save_ret != ESP_OK)
                {
                    ESP_LOGE(
                        TAG,
                        "Khong luu duoc event: %s",
                        esp_err_to_name(
                            save_ret
                        )
                    );
                }
            }


            /* ---------------------------------------------
             * Giam bo dem giu canh bao
             *
             * PHAI nam ngoai if(new_event)
             * --------------------------------------------- */

            if (lcd_event_hold_samples > 0)
            {
                lcd_event_hold_samples--;
            }


            /* ---------------------------------------------
             * LCD = 5 Hz
             *
             * 100 Hz / 20 = 5 Hz
             * --------------------------------------------- */

            lcd_counter++;

            if (lcd_counter >= 20)
            {
                lcd_counter = 0;

                if (lcd_event_hold_samples > 0)
                {
                    lcd1602_print_lines(
                        lcd_event_line1,
                        lcd_event_line2
                    );
                }
                else
                {
                    update_lcd_status(
                        &sensor,
                        &detection
                    );
                }
            }
        }
        else
        {
            ESP_LOGE(
                TAG,
                "Loi doc IMU: %s",
                esp_err_to_name(
                    ret
                )
            );
        }

        /*
        * =============================================
        * MQTT TELEMETRY
        *
        * Sensor 100 Hz
        * 100 samples = 1 giay
        * =============================================
        */

        if (telemetry_counter >= 100)
        {
            telemetry_counter = 0;


            if (
                mqtt_manager_is_connected()
            )
            {
                mqtt_manager_publish_telemetry(
                    &sensor,
                    &detection,
                    wifi_manager_get_rssi()
                );
            }
        }

        /* =====================================================
        * WIFI FAILOVER
        *
        * Chi kiem tra khi dang o che do mang binh thuong.
        *
        * Sensor = 100 Hz
        * 3000 sample = 30 giay.
        * ===================================================== */

        if (
            app_config.configured &&
            !force_setup_mode
        )
        {
            if (
                wifi_manager_is_connected()
            )
            {
                /*
                * Co Wi-Fi -> reset bo dem.
                */
                wifi_offline_counter = 0;
            }
            else
            {
                wifi_offline_counter++;


                /*
                * Mat Wi-Fi lien tuc 30 giay.
                */
                if (
                    wifi_offline_counter >= 3000
                )
                {
                    ESP_LOGW(
                        TAG,
                        "WiFi offline 30 giay"
                    );


                    ESP_LOGW(
                        TAG,
                        "Chuyen sang Setup Mode"
                    );


                    lcd1602_print_lines(
                        "WIFI FAILED",
                        "SETUP MODE..."
                    );


                    ESP_ERROR_CHECK(
                        app_config_request_setup_mode()
                    );


                    /*
                    * Cho nguoi dung nhin LCD.
                    */
                    vTaskDelay(
                        pdMS_TO_TICKS(1500)
                    );


                    esp_restart();
                }
            }
        }

        /* ---------------------------------------------
         * Sensor = 100 Hz
         * --------------------------------------------- */

        vTaskDelayUntil(
            &last_wake_time,
            pdMS_TO_TICKS(10)
        );
    }
}
