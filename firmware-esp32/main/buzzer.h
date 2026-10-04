#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"

typedef enum
{
    BUZZER_ALERT_NONE = 0,
    BUZZER_ALERT_LIGHT,
    BUZZER_ALERT_MEDIUM,
    BUZZER_ALERT_STRONG,
    BUZZER_ALERT_DROP

} buzzer_alert_t;


esp_err_t buzzer_init(void);

void buzzer_alert(
    buzzer_alert_t alert
);

#endif