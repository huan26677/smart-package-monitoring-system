#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"


esp_err_t time_manager_init(void);

bool time_manager_is_synced(void);

int64_t time_manager_get_epoch(void);

void time_manager_get_string(
    char *buffer,
    size_t buffer_size
);

void time_manager_format_epoch(
    int64_t timestamp,
    char *buffer,
    size_t buffer_size
);

#endif