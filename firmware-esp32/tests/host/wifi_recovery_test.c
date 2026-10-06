#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "network_stubs/common.h"

/* Exercise the actual manager against simulated ESP-IDF events and time. */
#include "../../main/wifi_manager.c"
const char *WIFI_EVENT="wifi", *IP_EVENT="ip";
static int64_t clock_ms;
static unsigned attempts, portal_calls, stop_ms, scenario;
static bool portal_active, fail_portal_once;
static esp_err_t connect_result=ESP_FAIL;
static event_handler_t handler;
static void (*watch_task)(void*);
static wifi_config_t saved_wifi;
static jmp_buf completed;

esp_err_t esp_netif_init(void){return ESP_OK;}
esp_err_t esp_event_loop_create_default(void){return ESP_OK;}
void *esp_netif_create_default_wifi_sta(void){return (void*)1;}
void *esp_netif_create_default_wifi_ap(void){return (void*)2;}
esp_err_t esp_wifi_init(const wifi_init_config_t *c){(void)c;return ESP_OK;}
esp_err_t esp_event_handler_register(esp_event_base_t b,int32_t id,event_handler_t h,void *a){(void)b;(void)id;(void)a;handler=h;return ESP_OK;}
esp_err_t esp_wifi_connect(void){attempts++;return connect_result;}
esp_err_t esp_wifi_disconnect(void){handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);return ESP_OK;}
esp_err_t esp_wifi_set_storage(int s){assert(s==WIFI_STORAGE_RAM);return ESP_OK;}
esp_err_t esp_wifi_set_mode(int m){assert(m==WIFI_MODE_STA);return ESP_OK;}
esp_err_t esp_wifi_set_config(int i,const wifi_config_t *c){assert(i==WIFI_IF_STA);saved_wifi=*c;return ESP_OK;}
esp_err_t esp_wifi_start(void){handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_START,NULL);return ESP_OK;}
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap){ap->rssi=-42;return ESP_OK;}
int64_t esp_timer_get_time(void){return clock_ms*1000;}
bool setup_portal_is_active(void){return portal_active;}
esp_err_t setup_portal_start(void){portal_calls++;if(fail_portal_once){fail_portal_once=false;return ESP_FAIL;}portal_active=true;return ESP_OK;}
int xTaskCreate(void (*task)(void*),const char *name,unsigned stack,void *arg,unsigned priority,void *handle){
    (void)stack;(void)arg;(void)priority;(void)handle;assert(!strcmp(name,"network_watch"));watch_task=task;return pdPASS;
}
void vTaskDelay(unsigned ticks){
    clock_ms+=ticks;
    if(scenario && clock_ms==10000) esp_wifi_disconnect();
    if(scenario==1 && clock_ms==65000) handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);
    if(clock_ms>=stop_ms)longjmp(completed,1);
}
static void reset(void){
    clock_ms=0;attempts=0;portal_calls=0;portal_active=false;fail_portal_once=false;scenario=0;
    connect_result=ESP_FAIL;wifi_connected=false;connecting=false;associated=false;
    memset(&saved_wifi,0,sizeof(saved_wifi));
}
static void run(unsigned milliseconds){stop_ms=milliseconds;if(!setjmp(completed))watch_task(NULL);}
int main(void){
    reset();assert(wifi_manager_init("missing-network","12345678")==ESP_OK);
    assert(attempts==1);run(60000);assert(!portal_active && attempts==12);
    reset();assert(wifi_manager_init("missing-network","12345678")==ESP_OK);run(65000);
    assert(portal_active && portal_calls==1 && attempts==13 && !wifi_manager_is_connected());

    reset();assert(wifi_manager_init("","")==ESP_OK);fail_portal_once=true;run(64000);
    assert(attempts==0 && portal_active && portal_calls==2);

    reset();assert(wifi_manager_init("working-network","12345678")==ESP_OK);
    handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);assert(wifi_manager_get_rssi()==-42);
    scenario=1;run(120000);assert(wifi_manager_is_connected() && !portal_active && attempts>1);

    reset();assert(wifi_manager_init("working-network","12345678")==ESP_OK);
    handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);scenario=2;run(75000);
    assert(!wifi_manager_is_connected() && portal_active && portal_calls==1);

    reset();connect_result=ESP_OK;assert(wifi_manager_init("dhcp-pending","12345678")==ESP_OK);run(65000);
    assert(attempts==1 && portal_active);handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);
    assert(wifi_manager_is_connected() && setup_portal_is_active());

    reset();char ssid[33],password[65];memset(ssid,'s',32);ssid[32]=0;memset(password,'a',64);password[64]=0;
    assert(wifi_manager_init(ssid,password)==ESP_OK);
    assert(!memcmp(saved_wifi.sta.ssid,ssid,32) && !memcmp(saved_wifi.sta.password,password,64));
    reset();assert(wifi_manager_init("open-network","")==ESP_OK);assert(saved_wifi.sta.threshold.authmode==WIFI_AUTH_OPEN);
    assert(wifi_manager_init("test","short")==ESP_ERR_INVALID_ARG);
    puts("Wi-Fi recovery: retry pacing, 60s setup, portal failure retry, connection recovery, DHCP wait and credential limits passed");
    return 0;
}
