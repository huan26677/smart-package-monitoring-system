#include "lcd1602.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"


#define LCD_RS GPIO_NUM_4
#define LCD_E  GPIO_NUM_5

#define LCD_D4 GPIO_NUM_6
#define LCD_D5 GPIO_NUM_7
#define LCD_D6 GPIO_NUM_15
#define LCD_D7 GPIO_NUM_16


static void lcd_pulse_enable(void)
{
    gpio_set_level(LCD_E, 1);

    esp_rom_delay_us(1);

    gpio_set_level(LCD_E, 0);

    esp_rom_delay_us(50);
}


static void lcd_write_nibble(
    uint8_t nibble
)
{
    gpio_set_level(
        LCD_D4,
        (nibble >> 0) & 0x01
    );

    gpio_set_level(
        LCD_D5,
        (nibble >> 1) & 0x01
    );

    gpio_set_level(
        LCD_D6,
        (nibble >> 2) & 0x01
    );

    gpio_set_level(
        LCD_D7,
        (nibble >> 3) & 0x01
    );

    lcd_pulse_enable();
}


static void lcd_send(
    uint8_t value,
    bool data_mode
)
{
    gpio_set_level(
        LCD_RS,
        data_mode ? 1 : 0
    );

    /*
     * Gui 4 bit cao truoc
     */
    lcd_write_nibble(
        value >> 4
    );

    /*
     * Sau do 4 bit thap
     */
    lcd_write_nibble(
        value & 0x0F
    );
}


static void lcd_command(
    uint8_t command
)
{
    lcd_send(
        command,
        false
    );

    /*
     * Clear va Return Home
     * can nhieu thoi gian hon.
     */
    if (
        command == 0x01 ||
        command == 0x02
    )
    {
        esp_rom_delay_us(2000);
    }
}


static void lcd_data(
    uint8_t data
)
{
    lcd_send(
        data,
        true
    );
}


esp_err_t lcd1602_init(void)
{
    gpio_config_t io_conf =
    {
        .pin_bit_mask =
            (1ULL << LCD_RS) |
            (1ULL << LCD_E)  |
            (1ULL << LCD_D4) |
            (1ULL << LCD_D5) |
            (1ULL << LCD_D6) |
            (1ULL << LCD_D7),

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
            &io_conf
        );


    if (ret != ESP_OK)
    {
        return ret;
    }


    gpio_set_level(
        LCD_RS,
        0
    );

    gpio_set_level(
        LCD_E,
        0
    );


    /*
     * Cho LCD khoi dong.
     */
    esp_rom_delay_us(50000);


    /*
     * Startup sequence cua HD44780.
     */

    lcd_write_nibble(0x03);
    esp_rom_delay_us(5000);

    lcd_write_nibble(0x03);
    esp_rom_delay_us(200);

    lcd_write_nibble(0x03);
    esp_rom_delay_us(200);


    /*
     * Chuyen sang che do 4-bit.
     */
    lcd_write_nibble(0x02);


    /*
     * Function Set
     *
     * 4 bit
     * 2 dong
     * font 5x8
     */
    lcd_command(0x28);


    /*
     * Display ON
     * Cursor OFF
     * Blink OFF
     */
    lcd_command(0x0C);


    /*
     * Entry Mode
     *
     * Cursor tang sang phai.
     */
    lcd_command(0x06);


    /*
     * Clear display.
     */
    lcd1602_clear();


    return ESP_OK;
}


void lcd1602_clear(void)
{
    lcd_command(0x01);
}


void lcd1602_set_cursor(
    int row,
    int column
)
{
    uint8_t address;


    if (row == 0)
    {
        address =
            0x00 + column;
    }
    else
    {
        address =
            0x40 + column;
    }


    lcd_command(
        0x80 | address
    );
}


void lcd1602_print(
    const char *text
)
{
    while (*text)
    {
        lcd_data(
            (uint8_t)*text
        );

        text++;
    }
}


static void lcd_print_fixed_16(
    const char *text
)
{
    int i = 0;


    while (
        text[i] != '\0' &&
        i < 16
    )
    {
        lcd_data(
            (uint8_t)text[i]
        );

        i++;
    }


    /*
     * Ghi khoang trang het phan con lai
     * de xoa chu cu.
     */

    while (i < 16)
    {
        lcd_data(' ');

        i++;
    }
}


void lcd1602_print_lines(
    const char *line1,
    const char *line2
)
{
    lcd1602_set_cursor(
        0,
        0
    );

    lcd_print_fixed_16(
        line1
    );


    lcd1602_set_cursor(
        1,
        0
    );

    lcd_print_fixed_16(
        line2
    );
}