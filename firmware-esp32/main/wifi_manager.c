#include "wifi_manager.h"
#include "network_config.h"
#include "setup_portal.h"
#include <string.h>
#include <stdatomic.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RETRY_MS 5000
#define SETUP_AFTER_MS 60000
static const char *TAG="WIFI";
static atomic_bool wifi_connected=false, connecting=false, associated=false;
static bool station_configured=false;

static void connect_station(void) {
    if(!station_configured || connecting || associated) return;
    connecting=true;
    esp_err_t ret=esp_wifi_connect();
    if(ret!=ESP_OK) {connecting=false;ESP_LOGW(TAG,"Thu ket noi Wi-Fi: %s",esp_err_to_name(ret));}
}

static void wifi_event_handler(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START) connect_station();
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_CONNECTED) associated=true;
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected=false; associated=false; connecting=false;
        const wifi_event_sta_disconnected_t *event=data;
        ESP_LOGW(TAG,"Wi-Fi mat ket noi (ma %u); se tu thu lai",event?event->reason:0);
    } else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        wifi_connected=true; associated=true; connecting=false;
        const ip_event_got_ip_t *event=data;
        ESP_LOGI(TAG,"Wi-Fi da ket noi, IP: " IPSTR,IP2STR(&event->ip_info.ip));
    } else if(base==IP_EVENT && id==IP_EVENT_STA_LOST_IP) {
        wifi_connected=false; associated=false; connecting=false;
        esp_wifi_disconnect();
    }
}

static void network_watch_task(void *arg) {
    (void)arg;
    int64_t offline_since=esp_timer_get_time()/1000, last_attempt=offline_since;
    while(1) {
        int64_t now=esp_timer_get_time()/1000;
        if(wifi_connected) offline_since=now;
        else {
            if(now-last_attempt>=RETRY_MS) {connect_station();last_attempt=now;}
            if(now-offline_since>=SETUP_AFTER_MS && !setup_portal_is_active()) {
                esp_err_t ret=setup_portal_start();
                if(ret!=ESP_OK) ESP_LOGW(TAG,"Chua mo duoc trang cau hinh: %s",esp_err_to_name(ret));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t wifi_manager_init(const char *ssid,const char *password) {
    if(!ssid || !password || strlen(ssid)>32 || !network_wifi_password_valid(password)) return ESP_ERR_INVALID_ARG;
    station_configured=ssid[0]!='\0';
    esp_err_t ret=esp_netif_init();
    if(ret!=ESP_OK && ret!=ESP_ERR_INVALID_STATE) return ret;
    ret=esp_event_loop_create_default();
    if(ret!=ESP_OK && ret!=ESP_ERR_INVALID_STATE) return ret;
    if(!esp_netif_create_default_wifi_sta() || !esp_netif_create_default_wifi_ap()) return ESP_ERR_NO_MEM;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    ret=esp_wifi_init(&init); if(ret!=ESP_OK) return ret;
    ret=esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_handler,NULL); if(ret!=ESP_OK) return ret;
    ret=esp_event_handler_register(IP_EVENT,ESP_EVENT_ANY_ID,wifi_event_handler,NULL); if(ret!=ESP_OK) return ret;
    ret=esp_wifi_set_storage(WIFI_STORAGE_RAM); if(ret!=ESP_OK) return ret;
    wifi_config_t config={0};
    config.sta.threshold.authmode=password[0]?WIFI_AUTH_WPA2_PSK:WIFI_AUTH_OPEN;
    config.sta.sae_pwe_h2e=WPA3_SAE_PWE_BOTH;
    memcpy(config.sta.ssid,ssid,strlen(ssid));
    memcpy(config.sta.password,password,strlen(password));
    ret=esp_wifi_set_mode(WIFI_MODE_STA); if(ret!=ESP_OK) return ret;
    if(station_configured) {ret=esp_wifi_set_config(WIFI_IF_STA,&config);if(ret!=ESP_OK) return ret;}
    ret=esp_wifi_start(); if(ret!=ESP_OK) return ret;
    return xTaskCreate(network_watch_task,"network_watch",4096,NULL,1,NULL)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}

bool wifi_manager_is_connected(void) {return wifi_connected;}
int8_t wifi_manager_get_rssi(void) {
    wifi_ap_record_t ap;
    return wifi_connected && esp_wifi_sta_get_ap_info(&ap)==ESP_OK?ap.rssi:-127;
}
