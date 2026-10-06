#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_config.h"
#include "nvs.h"

static const char *keys[] = {"wifi_ssid", "wifi_pass", "mqtt_uri", "mqtt_user", "mqtt_pass"};
static char values[5][400];
static int absent = -1, wrong_type = -1, erased = 0, writes = 0;
static esp_err_t read_error = ESP_OK, flag_error = ESP_OK;
static uint8_t configured = 1;

static int index_of(const char *key) {
    for (int i=0;i<5;i++) if (!strcmp(key, keys[i])) return i;
    return -1;
}
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle) {
    (void)ns; (void)mode; *handle=1; return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out) {
    (void)handle; (void)key; *out=configured; return flag_error;
}
esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out, size_t *size) {
    (void)handle; int i=index_of(key); assert(i>=0);
    if (read_error!=ESP_OK) return read_error;
    if (i==absent) return ESP_ERR_NVS_NOT_FOUND;
    if (i==wrong_type) return ESP_ERR_NVS_TYPE_MISMATCH;
    size_t required=strlen(values[i])+1;
    if (*size<required) { *size=required; return ESP_ERR_NVS_INVALID_LENGTH; }
    memcpy(out,values[i],required); *size=required; return ESP_OK;
}
esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value) {
    (void)handle; int i=index_of(key); assert(i>=0);
    strcpy(values[i],value); if(i==absent)absent=-1; if(i==wrong_type)wrong_type=-1;
    writes++; return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) {
    (void)handle; if(!strcmp(key,"configured")){configured=value;flag_error=ESP_OK;}
    writes++;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle) { (void)handle; return ESP_OK; }
esp_err_t nvs_erase_all(nvs_handle_t handle) { (void)handle; erased++; return ESP_OK; }

static void fixture(void) {
    strcpy(values[0],"Demo WiFi"); strcpy(values[1],"wifi-fixture-only");
    strcpy(values[2],"wss://mqtt.example.com/mqtt"); strcpy(values[3],"esp32-001");
    strcpy(values[4],"mqtt-fixture-only"); absent=wrong_type=-1;
    read_error=flag_error=ESP_OK; configured=1; erased=writes=0;
}
int main(void) {
    assert(app_config_init()==ESP_OK);
    app_config_t config; esp_err_t stored;
    fixture(); assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_OK && config.configured && !strcmp(config.wifi_password,values[1]));

    fixture(); absent=1;
    assert(app_config_load(&config)==ESP_ERR_NVS_NOT_FOUND);
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_ERR_NVS_NOT_FOUND && !config.configured);
    assert(!strcmp(config.wifi_ssid,"Demo WiFi") && !strcmp(config.mqtt_username,"esp32-001"));
    assert(!config.wifi_password[0] && !config.mqtt_password[0] && !erased && !writes);
    strcpy(config.wifi_password,"new-fixture-wifi"); strcpy(config.mqtt_password,"new-fixture-mqtt");
    assert(app_config_save(&config)==ESP_OK);
    assert(app_config_load(&config)==ESP_OK && config.configured && !erased);

    fixture(); absent=2;
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_ERR_NVS_NOT_FOUND && !config.mqtt_broker[0]);
    assert(!strcmp(config.wifi_ssid,"Demo WiFi") && !config.wifi_password[0]);
    assert(!writes && !erased);

    fixture(); memset(values[2],'x',300); values[2][300]=0;
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_ERR_NVS_INVALID_LENGTH && !config.mqtt_broker[0]);
    assert(!strcmp(config.mqtt_username,"esp32-001") && !erased && !writes);

    fixture(); wrong_type=0;
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_ERR_NVS_TYPE_MISMATCH && !config.wifi_ssid[0] && !writes);

    fixture(); flag_error=ESP_ERR_NVS_TYPE_MISMATCH;
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(stored==ESP_ERR_NVS_TYPE_MISMATCH && !strcmp(config.wifi_ssid,"Demo WiFi"));

    fixture(); read_error=ESP_FAIL;
    assert(app_config_load_for_setup(&config,&stored)==ESP_FAIL && !writes && !erased);

    fixture(); absent=1; strcpy(values[2],"wss://legacy:fixture-secret@mqtt.example.com/mqtt");
    assert(app_config_load_for_setup(&config,&stored)==ESP_OK);
    assert(!strcmp(config.mqtt_broker,"wss://mqtt.example.com/mqtt"));
    assert(!strcmp(config.mqtt_username,"legacy") && !config.mqtt_password[0]);
    assert(!writes && !erased);
    puts("PASS: setup recovers missing/oversized/wrong-type config, preserves NVS, requires fresh passwords, rejects storage errors.");
    return 0;
}
