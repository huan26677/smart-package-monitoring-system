#include "setup_portal.h"
#include "app_config.h"
#include "network_config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include "cJSON.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SETUP_AP_SSID "SMART_PACKAGE_SETUP"
#define SETUP_AP_PASSWORD "smart1234"
#define POST_MAX 3072
static const char *TAG="SETUP_PORTAL";
static httpd_handle_t server=NULL;
static atomic_bool active=false, starting=false;
static char token[33];
extern const uint8_t page_start[] asm("_binary_setup_portal_html_start");
extern const uint8_t page_end[] asm("_binary_setup_portal_html_end");

bool setup_portal_is_active(void) {return active;}
static esp_err_t json_response(httpd_req_t *req,cJSON *object,const char *status) {
    if(!object) return ESP_ERR_NO_MEM;
    char *body=cJSON_PrintUnformatted(object);cJSON_Delete(object);
    if(!body) return ESP_ERR_NO_MEM;
    httpd_resp_set_status(req,status);
    httpd_resp_set_type(req,"application/json; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    esp_err_t ret=httpd_resp_send(req,body,HTTPD_RESP_USE_STRLEN);free(body);return ret;
}
static esp_err_t error(httpd_req_t *req,const char *message,const char *status) {
    cJSON *o=cJSON_CreateObject();if(o)cJSON_AddStringToObject(o,"message",message);
    return json_response(req,o,status);
}
static esp_err_t config_error(httpd_req_t *req,esp_err_t code) {
    cJSON *o=cJSON_CreateObject();
    if(o){cJSON_AddStringToObject(o,"message","Không truy cập được bộ nhớ cấu hình. Hãy khởi động lại ESP32; nếu vẫn lỗi, kiểm tra thiết bị qua USB.");
        cJSON_AddStringToObject(o,"config_error",esp_err_to_name(code));}
    return json_response(req,o,"500 Internal Server Error");
}
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
    return httpd_resp_send(req,(const char*)page_start,(ssize_t)(page_end-page_start-1));
}
static esp_err_t status_handler(httpd_req_t *req) {
    app_config_t config;
    esp_err_t stored_error,ret=app_config_load_for_setup(&config,&stored_error);
    if(ret!=ESP_OK) return config_error(req,ret);
    cJSON *o=cJSON_CreateObject();if(!o)return ESP_ERR_NO_MEM;
    cJSON_AddStringToObject(o,"ssid",config.wifi_ssid);
    cJSON_AddStringToObject(o,"broker",config.mqtt_broker[0]?config.mqtt_broker:"wss://mqtt.huan2k5.id.vn/mqtt");
    cJSON_AddStringToObject(o,"mqtt_user",config.mqtt_username[0]?config.mqtt_username:(config.configured?"":"esp32-001"));
    cJSON_AddBoolToObject(o,"config_recovery",stored_error!=ESP_OK);
    if(stored_error!=ESP_OK) {
        cJSON_AddStringToObject(o,"config_error",esp_err_to_name(stored_error));
        cJSON_AddStringToObject(o,"config_warning","Cấu hình cũ thiếu hoặc sai định dạng. Hãy nhập lại mật khẩu Wi-Fi và MQTT rồi lưu; lịch sử sự kiện được giữ nguyên.");
    }
    cJSON_AddBoolToObject(o,"has_wifi_password",config.wifi_password[0]!=0);
    cJSON_AddBoolToObject(o,"has_mqtt_password",config.mqtt_password[0]!=0);
    cJSON_AddBoolToObject(o,"wifi_connected",wifi_manager_is_connected());
    cJSON_AddStringToObject(o,"mqtt_status",mqtt_manager_status());
    cJSON_AddStringToObject(o,"token",token);
    return json_response(req,o,"200 OK");
}
static void restart_task(void *arg) {(void)arg;vTaskDelay(pdMS_TO_TICKS(1800));esp_restart();}

static esp_err_t save_handler(httpd_req_t *req) {
    if(req->content_len<=0 || req->content_len>=POST_MAX) return error(req,"Nội dung cấu hình quá dài hoặc trống.","400 Bad Request");
    char *body=calloc((size_t)req->content_len+1,1);
    if(!body) return error(req,"Thiết bị thiếu bộ nhớ. Hãy thử lại.","503 Service Unavailable");
    int n=0,timeouts=0;
    while(n<req->content_len) {
        int r=httpd_req_recv(req,body+n,req->content_len-n);
        if(r==HTTPD_SOCK_ERR_TIMEOUT && timeouts++<3)continue;
        if(r<=0){free(body);return error(req,"Không nhận đủ dữ liệu cấu hình.","400 Bad Request");}
        n+=r;
    }
    app_config_t config,old;
    esp_err_t stored_error,read_ret=app_config_load_for_setup(&old,&stored_error);
    if(read_ret!=ESP_OK){free(body);return config_error(req,read_ret);}
    config=old;
    char submitted_token[33],ssid[33],wifi_pass[65],uri[NETWORK_URI_MAX+1];
    char user[NETWORK_USER_MAX+1],secret[NETWORK_SECRET_MAX+1],open_wifi[2],clear_mqtt[2];
    int a=network_form_value(body,"token",submitted_token,sizeof(submitted_token));
    int b=network_form_value(body,"ssid",ssid,sizeof(ssid));
    int c=network_form_value(body,"password",wifi_pass,sizeof(wifi_pass));
    int d=network_form_value(body,"broker",uri,sizeof(uri));
    int e=network_form_value(body,"mqtt_user",user,sizeof(user));
    int f=network_form_value(body,"mqtt_pass",secret,sizeof(secret));
    int g=network_form_value(body,"open_wifi",open_wifi,sizeof(open_wifi));
    int h=network_form_value(body,"clear_mqtt",clear_mqtt,sizeof(clear_mqtt));
    free(body);
    if(a!=1 || strcmp(submitted_token,token)) return error(req,"Phiên cấu hình không hợp lệ. Hãy tải lại trang.","403 Forbidden");
    if(b!=1 || !ssid[0] || c<0 || d<0 || e<0 || f<0 || g<0 || h<0)
        return error(req,"Tên mạng hoặc dữ liệu cấu hình không hợp lệ, quá dài hoặc lặp trường.","400 Bad Request");
    strcpy(config.wifi_ssid,ssid);
    if(g==1 && !strcmp(open_wifi,"1")) config.wifi_password[0]=0;
    else if(wifi_pass[0]) strcpy(config.wifi_password,wifi_pass);
    else if(stored_error!=ESP_OK || strcmp(old.wifi_ssid,ssid)) return error(req,"Nhập mật khẩu mạng mới, hoặc chọn mạng Wi-Fi không có mật khẩu.","400 Bad Request");
    if(!network_wifi_password_valid(config.wifi_password)) return error(req,"Mật khẩu Wi-Fi cần 8–63 ký tự hoặc 64 chữ số hex.","400 Bad Request");
    if(d==1 && uri[0]) strcpy(config.mqtt_broker,uri);
    network_broker_t broker;
    if(!network_parse_broker(config.mqtt_broker,&broker)) return error(req,"Địa chỉ MQTT không hợp lệ. Dùng mqtt://, mqtts://, ws:// hoặc wss:// và tên máy chủ.","400 Bad Request");
    strcpy(config.mqtt_broker,broker.uri);
    if(broker.has_credentials){strcpy(config.mqtt_username,broker.username);strcpy(config.mqtt_password,broker.password);}
    else if(e==1) {
        strcpy(config.mqtt_username,user);
        if(strcmp(old.mqtt_username,user) && !secret[0] && !(h==1 && !strcmp(clear_mqtt,"1")))
            return error(req,"Bạn đang đổi tài khoản MQTT. Hãy nhập mật khẩu mới.","400 Bad Request");
    }
    if(secret[0])strcpy(config.mqtt_password,secret);
    if(h==1 && !strcmp(clear_mqtt,"1")){config.mqtt_username[0]=0;config.mqtt_password[0]=0;}
    if(stored_error!=ESP_OK && config.mqtt_username[0] && !config.mqtt_password[0])
        return error(req,"Cấu hình cũ bị lỗi. Hãy nhập lại mật khẩu MQTT.","400 Bad Request");
    if(!config.mqtt_username[0] && config.mqtt_password[0]) return error(req,"Nhập tài khoản MQTT hoặc chọn máy chủ không dùng tài khoản.","400 Bad Request");
    config.configured=true;
    esp_err_t ret=app_config_save(&config);
    if(ret!=ESP_OK) {ESP_LOGE(TAG,"Luu cau hinh that bai: %s",esp_err_to_name(ret));return config_error(req,ret);}
    ESP_LOGI(TAG,"Da luu cau hinh mang; khoi dong lai, giu nguyen lich su su kien");
    ret=error(req,"Đã lưu. ESP32 đang khởi động lại để kết nối mạng mới. Bạn có thể rời SMART_PACKAGE_SETUP và mở lại dashboard. Nếu chưa kết nối sau 60 giây, mạng cấu hình sẽ tự xuất hiện.","200 OK");
    if(xTaskCreate(restart_task,"network_restart",2048,NULL,1,NULL)!=pdPASS)
        ESP_LOGE(TAG,"Khong tao duoc tac vu khoi dong lai; can khoi dong thu cong");
    return ret;
}

esp_err_t setup_portal_start(void) {
    if(active)return ESP_OK;
    bool expected=false;
    if(!atomic_compare_exchange_strong(&starting,&expected,true))return ESP_ERR_INVALID_STATE;
    esp_err_t ret;
    wifi_config_t ap={0};
    memcpy(ap.ap.ssid,SETUP_AP_SSID,strlen(SETUP_AP_SSID));
    memcpy(ap.ap.password,SETUP_AP_PASSWORD,strlen(SETUP_AP_PASSWORD));
    ap.ap.ssid_len=strlen(SETUP_AP_SSID);ap.ap.channel=1;ap.ap.max_connection=4;ap.ap.authmode=WIFI_AUTH_WPA2_PSK;
    ret=esp_wifi_set_mode(WIFI_MODE_APSTA);if(ret!=ESP_OK)goto failed;
    ret=esp_wifi_set_config(WIFI_IF_AP,&ap);if(ret!=ESP_OK)goto failed;
    unsigned char bytes[16];esp_fill_random(bytes,sizeof(bytes));
    for(size_t i=0;i<sizeof(bytes);i++)snprintf(token+2*i,3,"%02x",bytes[i]);
    httpd_config_t http=HTTPD_DEFAULT_CONFIG();http.stack_size=8192;http.lru_purge_enable=true;
    ret=httpd_start(&server,&http);if(ret!=ESP_OK)goto failed;
    httpd_uri_t root={.uri="/",.method=HTTP_GET,.handler=root_handler};
    httpd_uri_t status={.uri="/status",.method=HTTP_GET,.handler=status_handler};
    httpd_uri_t save={.uri="/save",.method=HTTP_POST,.handler=save_handler};
    ret=httpd_register_uri_handler(server,&root);if(ret!=ESP_OK)goto failed;
    ret=httpd_register_uri_handler(server,&status);if(ret!=ESP_OK)goto failed;
    ret=httpd_register_uri_handler(server,&save);if(ret!=ESP_OK)goto failed;
    active=true;starting=false;
    ESP_LOGW(TAG,"Cau hinh: SMART_PACKAGE_SETUP, http://192.168.4.1; van giam sat va thu lai Wi-Fi");
    return ESP_OK;
failed:
    if(server){httpd_stop(server);server=NULL;}
    esp_wifi_set_mode(WIFI_MODE_STA);
    starting=false;return ret;
}
