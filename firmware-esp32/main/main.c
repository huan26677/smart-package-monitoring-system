#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

#include "mpu6050.h"
#include "impact_detector.h"
#include "motion_collector.h"
#include "nvs_flash.h"
#include "event_log.h"
#include "esp_timer.h"
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


static const char *lcd_level_vi(impact_level_t level) {
    switch(level) {case IMPACT_LEVEL_LIGHT:return "NHE";case IMPACT_LEVEL_MEDIUM:return "VUA";case IMPACT_LEVEL_STRONG:return "MANH";default:return ""; }
}

static void update_lcd_status(
    const mpu6050_data_t *sensor,
    const impact_result_t *detection
)
{
    char line1[17] = {0};
    char line2[17] = {0};

    if (!detection->calibrated)
    {
        snprintf(line1, sizeof(line1), "LAY MOC CHUAN");
        snprintf(
            line2,
            sizeof(line2),
            "GIU YEN %d%%",
            detection->calibration_percent
        );
    }
    else if (detection->state == PACKAGE_NORMAL)
    {
        snprintf(line1, sizeof(line1), "BINH THUONG");
        snprintf(
            line2,
            sizeof(line2),
            "GOC:%3.0f DO",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_VIBRATION)
    {
        snprintf(line1, sizeof(line1), "RUNG LAC");
        snprintf(
            line2,
            sizeof(line2),
            "RMS:%.2fG",
            detection->vibration_rms
        );
    }
    else if (detection->state == PACKAGE_TILT)
    {
        snprintf(line1, sizeof(line1), "CANH BAO NGHIENG");
        snprintf(
            line2,
            sizeof(line2),
            "GOC:%3.0f DO",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_FLIP)
    {
        snprintf(line1, sizeof(line1), "KIEN BI LAT");
        snprintf(
            line2,
            sizeof(line2),
            "GOC:%3.0f DO",
            detection->relative_tilt_deg
        );
    }
    else if (detection->state == PACKAGE_FREE_FALL)
    {
        snprintf(line1, sizeof(line1), "ROI TU DO!");
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
            "VA DAP %s",
            lcd_level_vi(detection->impact_level)
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
        snprintf(line1, sizeof(line1), "KIEN BI ROI");
        snprintf(
            line2,
            sizeof(line2),
            "VA DAP:%.2fg",
            sensor->total_g
        );
    }
    else
    {
        snprintf(line1, sizeof(line1), "GIAM SAT KIEN");
        snprintf(line2, sizeof(line2), "DANG GIAM SAT");
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

    switch (detection->event_type)
    {
        case PACKAGE_IMPACT:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "VA DAP %s",
                lcd_level_vi(detection->event_level)
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "G:%.2f",
                detection->event_peak_g
            );
            break;


        case PACKAGE_DROP:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "KIEN BI ROI"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "VA DAP:%.2fg",
                detection->event_peak_g
            );
            break;


        case PACKAGE_FREE_FALL:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "ROI TU DO!"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "G:%.2f",
                detection->event_peak_g
            );
            break;


        case PACKAGE_TILT:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "CANH BAO NGHIENG"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "GOC:%3.0f DO",
                detection->event_angle
            );
            break;


        case PACKAGE_FLIP:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "KIEN BI LAT"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "GOC:%3.0f DO",
                detection->event_angle
            );
            break;


        case PACKAGE_VIBRATION:
            snprintf(
                lcd_event_line1,
                sizeof(lcd_event_line1),
                "RUNG LAC"
            );

            snprintf(
                lcd_event_line2,
                sizeof(lcd_event_line2),
                "RMS:%.2fG",
                detection->event_vibration
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


typedef struct {mpu6050_data_t sensor; impact_result_t detection;} monitor_snapshot_t;
static QueueHandle_t monitor_samples, monitor_alerts;
static void monitor_task(void *arg)
{
    (void)arg;
    mpu6050_data_t sensor = {0};
    impact_result_t detection = {0};

    uint64_t last_print_ms = 0, last_lcd_ms = 0, last_telemetry_ms = 0;
    uint64_t last_update_ms = 0;

    monitor_snapshot_t snapshot;

    /* =====================================================
     * MAIN LOOP
     * ===================================================== */

    while (1)
    {
        if (xQueueReceive(monitor_samples, &snapshot, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        sensor = snapshot.sensor;
        detection = snapshot.detection;
        detection.new_event = false;
        impact_result_t alert;
        if (xQueueReceive(monitor_alerts, &alert, 0) == pdTRUE) {
            detection.event_type = alert.event_type;
            detection.event_level = alert.event_level;
            detection.event_peak_g = alert.event_peak_g;
            detection.event_angle = alert.event_angle;
            detection.event_vibration = alert.event_vibration;
            detection.new_event = true;
        }
        uint64_t now_ms = esp_timer_get_time() / 1000ULL;
        if (last_update_ms && lcd_event_hold_samples > 0) {
            int elapsed = (int)((now_ms - last_update_ms) / 10);
            lcd_event_hold_samples = lcd_event_hold_samples > elapsed ? lcd_event_hold_samples - elapsed : 0;
        }
        last_update_ms = now_ms;
        {
            /* ---------------------------------------------
             * Terminal 1 Hz: serial output must not stall the 100 Hz sampler.
             * --------------------------------------------- */

            if (now_ms - last_print_ms >= 1000)
            {
                last_print_ms = now_ms;

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
                        detection.event_type
                    ),

                    impact_level_to_string(
                        detection.event_level
                    ),

                    detection.event_peak_g,

                    detection.event_angle,

                    detection.event_vibration
                );

                if (
                    detection.event_type ==
                    PACKAGE_DROP
                )
                {
                    buzzer_alert(
                        BUZZER_ALERT_DROP
                    );
                }

                else if (
                    detection.event_type ==
                    PACKAGE_IMPACT
                )
                {
                    switch (
                        detection.event_level
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


            }


            /* ---------------------------------------------
             * Giam bo dem giu canh bao
             *
             * PHAI nam ngoai if(new_event)
             * --------------------------------------------- */

            /* ---------------------------------------------
             * LCD = 5 Hz
             *
             * 100 Hz / 20 = 5 Hz
             * --------------------------------------------- */

            if (now_ms - last_lcd_ms >= 200)
            {
                last_lcd_ms = now_ms;

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
        /*
        * =============================================
        * MQTT TELEMETRY
        *
        * Sensor 100 Hz
        * 100 samples = 1 giay
        * =============================================
        */

        if (now_ms - last_telemetry_ms >= 1000)
        {
            last_telemetry_ms = now_ms;


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

    }
}


void app_main(void)
{
    /* =====================================================
     * NVS
     * ===================================================== */

    esp_err_t nvs_ret =
        nvs_flash_init();

    // Preserve configuration and unconfirmed events if NVS cannot be opened.
    // Recovery that erases flash must be an explicit maintenance operation.
    ESP_ERROR_CHECK(nvs_ret);

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
        "GIAM SAT KIEN",
        "DANG KHOI DONG"
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
            "CAU HINH",
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
    ESP_ERROR_CHECK(motion_collector_init());

    monitor_samples = xQueueCreate(1, sizeof(monitor_snapshot_t));
    monitor_alerts = xQueueCreate(32, sizeof(impact_result_t));
    ESP_ERROR_CHECK(monitor_samples && monitor_alerts ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(monitor_task, "monitor_ui", 4096, NULL, 1, NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    vTaskPrioritySet(NULL, 4);
    ESP_ERROR_CHECK(mpu6050_fifo_start());
    ESP_LOGI(TAG, "Lay mau FIFO 100 Hz; thoi gian mau uoc tinh tu bo dem");
    static mpu6050_sample_t samples[MPU6050_FIFO_MAX_SAMPLES];
    impact_result_t detection = {0};
    uint64_t last_sample_ms = 0;
    unsigned lost_batches = 0;
    while (1) {
        size_t count = 0;
        esp_err_t ret = mpu6050_fifo_read(samples, MPU6050_FIFO_MAX_SAMPLES, &count);
        if (ret == ESP_OK && count && samples[0].estimated_ms <= last_sample_ms)
            ret = ESP_ERR_INVALID_STATE;
        if (ret != ESP_OK) {
            /* Lost/uncertain packets never get interpolated into a good AI window. */
            motion_collector_discard_partial();
            ESP_LOGW(TAG, "Mat/loi FIFO #%u: %s; bo doan AI dang do", ++lost_batches, esp_err_to_name(ret));
            ESP_ERROR_CHECK(mpu6050_fifo_start());
            last_sample_ms = 0;
            vTaskDelay(1); // at least one scheduler tick, including CONFIG_FREERTOS_HZ=100
            continue;
        }
        for (size_t i = 0; i < count; ++i) {
            last_sample_ms = samples[i].estimated_ms;
            impact_detector_update(&samples[i].data, &detection, (uint32_t)last_sample_ms);
            motion_collector_add(&samples[i].data, &detection, last_sample_ms);
            if (detection.new_event) {
                esp_err_t saved = event_log_add(detection.event_type, detection.event_level,
                    detection.event_peak_g, detection.event_angle, detection.event_vibration,
                    detection.event_started_ms, detection.event_duration_ms, detection.event_saturated);
                if (saved != ESP_OK) ESP_LOGE(TAG, "Khong xep hang luu event: %s", esp_err_to_name(saved));
                /* Persistent event logging happens above even if display falls behind. */
                if (xQueueSend(monitor_alerts, &detection, 0) != pdTRUE)
                    ESP_LOGW(TAG, "Hang doi hien thi canh bao day");
            }
        }
        if (count) {
            monitor_snapshot_t snapshot = {.sensor = samples[count-1].data, .detection = detection};
            xQueueOverwrite(monitor_samples, &snapshot);
        }
        vTaskDelay(1); // at least one scheduler tick, including CONFIG_FREERTOS_HZ=100
    }
}
