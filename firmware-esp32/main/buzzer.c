#include "buzzer.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


#define BUZZER_GPIO GPIO_NUM_17


static QueueHandle_t buzzer_queue = NULL;


/* =========================================================
 * BEEP
 * ========================================================= */

static void buzzer_beep(
    int on_ms,
    int off_ms
)
{
    gpio_set_level(
        BUZZER_GPIO,
        1
    );

    vTaskDelay(
        pdMS_TO_TICKS(on_ms)
    );


    gpio_set_level(
        BUZZER_GPIO,
        0
    );


    if (off_ms > 0)
    {
        vTaskDelay(
            pdMS_TO_TICKS(off_ms)
        );
    }
}


/* =========================================================
 * BUZZER TASK
 * ========================================================= */

static void buzzer_task(
    void *parameter
)
{
    buzzer_alert_t alert;


    while (1)
    {
        if (
            xQueueReceive(
                buzzer_queue,
                &alert,
                portMAX_DELAY
            )
            == pdTRUE
        )
        {
            switch (alert)
            {
                case BUZZER_ALERT_LIGHT:

                    buzzer_beep(
                        100,
                        0
                    );

                    break;


                case BUZZER_ALERT_MEDIUM:

                    buzzer_beep(
                        120,
                        100
                    );

                    buzzer_beep(
                        120,
                        0
                    );

                    break;


                case BUZZER_ALERT_STRONG:

                    for (
                        int i = 0;
                        i < 3;
                        i++
                    )
                    {
                        buzzer_beep(
                            150,
                            100
                        );
                    }

                    break;


                case BUZZER_ALERT_DROP:

                    buzzer_beep(
                        600,
                        150
                    );

                    buzzer_beep(
                        200,
                        0
                    );

                    break;


                default:
                    break;
            }
        }
    }
}


/* =========================================================
 * INIT
 * ========================================================= */

esp_err_t buzzer_init(void)
{
    gpio_config_t config =
    {
        .pin_bit_mask =
            1ULL << BUZZER_GPIO,

        .mode =
            GPIO_MODE_OUTPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    esp_err_t ret =
        gpio_config(
            &config
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    gpio_set_level(
        BUZZER_GPIO,
        0
    );


    buzzer_queue =
        xQueueCreate(
            8,
            sizeof(buzzer_alert_t)
        );


    if (buzzer_queue == NULL)
    {
        return ESP_FAIL;
    }


    BaseType_t task_ret =
        xTaskCreate(
            buzzer_task,
            "buzzer_task",
            2048,
            NULL,
            5,
            NULL
        );


    if (task_ret != pdPASS)
    {
        return ESP_FAIL;
    }


    return ESP_OK;
}


/* =========================================================
 * ALERT
 * ========================================================= */

void buzzer_alert(
    buzzer_alert_t alert
)
{
    if (
        buzzer_queue == NULL ||
        alert == BUZZER_ALERT_NONE
    )
    {
        return;
    }


    xQueueSend(
        buzzer_queue,
        &alert,
        0
    );
}