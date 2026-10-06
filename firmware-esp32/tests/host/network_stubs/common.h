#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
typedef const char *esp_event_base_t;
extern const char *WIFI_EVENT, *IP_EVENT;
enum {WIFI_EVENT_STA_START=1,WIFI_EVENT_STA_CONNECTED,WIFI_EVENT_STA_DISCONNECTED,
      IP_EVENT_STA_GOT_IP=10,IP_EVENT_STA_LOST_IP};
#define ESP_EVENT_ANY_ID (-1)
#define WIFI_STORAGE_RAM 0
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WIFI_AUTH_OPEN 0
#define WIFI_AUTH_WPA2_PSK 3
#define WPA3_SAE_PWE_BOTH 3
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})
typedef struct {int unused;} wifi_init_config_t;
typedef struct {struct {uint8_t ssid[32],password[64];struct {int authmode;} threshold;int sae_pwe_h2e;} sta;} wifi_config_t;
typedef struct {uint8_t reason;} wifi_event_sta_disconnected_t;
typedef struct {struct {int ip;} ip_info;} ip_event_got_ip_t;
typedef struct {int8_t rssi;} wifi_ap_record_t;
#define IPSTR "%d"
#define IP2STR(ip) 0
typedef void (*event_handler_t)(void*,esp_event_base_t,int32_t,void*);
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t,int32_t,event_handler_t,void*);
esp_err_t esp_netif_init(void);
void *esp_netif_create_default_wifi_sta(void);
void *esp_netif_create_default_wifi_ap(void);
esp_err_t esp_wifi_init(const wifi_init_config_t*);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_set_storage(int);
esp_err_t esp_wifi_set_mode(int);
esp_err_t esp_wifi_set_config(int,const wifi_config_t*);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t*);
#define pdPASS 1
#define pdMS_TO_TICKS(ms) (ms)
int xTaskCreate(void (*task)(void*),const char*,unsigned,void*,unsigned,void*);
void vTaskDelay(unsigned);
