#ifndef SETUP_PORTAL_H
#define SETUP_PORTAL_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t setup_portal_start(void);
bool setup_portal_is_active(void);

#endif
