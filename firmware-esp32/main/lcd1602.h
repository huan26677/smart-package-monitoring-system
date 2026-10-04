#ifndef LCD1602_H
#define LCD1602_H

#include "esp_err.h"

esp_err_t lcd1602_init(void);

void lcd1602_clear(void);

void lcd1602_set_cursor(
    int row,
    int column
);

void lcd1602_print(
    const char *text
);

void lcd1602_print_lines(
    const char *line1,
    const char *line2
);

#endif