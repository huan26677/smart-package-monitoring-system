#include "network_console.h"
#include "network_config.h"
#include "app_config.h"
#include "setup_portal.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "event_log.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* USB maintenance commands never print credentials or erase event history. */
static void command(const char *line) {
    if(!strcmp(line,"NETSTATUS")) {
        printf("NETSTATUS wifi=%d mqtt=%d setup=%d pending=%u rejected=%u\n",
            wifi_manager_is_connected(),mqtt_manager_is_connected(),setup_portal_is_active(),
            (unsigned)event_log_unsynced_count(),(unsigned)event_log_rejected_count());
        return;
    }
    if(!strcmp(line,"NETSETUP")) {
        esp_err_t ret=setup_portal_start();
        printf("NETSETUP result=%s\n",esp_err_to_name(ret));return;
    }
    if(strncmp(line,"NETCFG ",7)) return;
    cJSON *o=cJSON_Parse(line+7);
    cJSON *uri=cJSON_GetObjectItemCaseSensitive(o,"broker");
    cJSON *user=cJSON_GetObjectItemCaseSensitive(o,"username");
    cJSON *secret=cJSON_GetObjectItemCaseSensitive(o,"password");
    app_config_t config;network_broker_t broker;
    bool valid=cJSON_IsString(uri) && cJSON_IsString(user) && cJSON_IsString(secret) &&
        strlen(user->valuestring)<=APP_CONFIG_MQTT_USER_MAX && strlen(secret->valuestring)<=APP_CONFIG_MQTT_PASS_MAX &&
        network_parse_broker(uri->valuestring,&broker) && !broker.has_credentials && app_config_load(&config)==ESP_OK;
    if(!valid) {cJSON_Delete(o);puts("NETCFG invalid");return;}
    strcpy(config.mqtt_broker,broker.uri);strcpy(config.mqtt_username,user->valuestring);strcpy(config.mqtt_password,secret->valuestring);
    esp_err_t ret=app_config_save(&config);
    memset(config.mqtt_password,0,sizeof(config.mqtt_password));cJSON_Delete(o);
    printf("NETCFG result=%s\n",esp_err_to_name(ret));fflush(stdout);
    if(ret==ESP_OK){vTaskDelay(pdMS_TO_TICKS(1500));esp_restart();}
}
static void console_task(void *arg) {
    (void)arg;
    char line[1024];size_t len=0;bool overflow=false;
    while(1) {
        int c=getchar();
        if(c<0){clearerr(stdin);vTaskDelay(pdMS_TO_TICKS(20));continue;}
        if(c=='\r' || c=='\n') {
            if(len && !overflow){line[len]=0;command(line);}
            memset(line,0,sizeof(line));len=0;overflow=false;
        } else if(len+1<sizeof(line)) line[len++]=(char)c;
        else overflow=true;
    }
}
esp_err_t network_console_start(void) {
    return xTaskCreate(console_task,"network_usb",8192,NULL,1,NULL)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}
