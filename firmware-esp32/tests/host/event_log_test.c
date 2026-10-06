#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nvs.h"
#include "../../main/event_log.c"
static unsigned char blob[20000];
static size_t blob_size;
static int fail_save;
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h) {(void)name;(void)mode;*h=1;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *data,size_t *size) {
    (void)h;(void)key;if(!blob_size)return ESP_ERR_NVS_NOT_FOUND;
    if(data) {assert(*size>=blob_size);memcpy(data,blob,blob_size);}*size=blob_size;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size) {
    (void)h;(void)key;if(fail_save)return ESP_FAIL;
    memcpy(blob,data,size);blob_size=size;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {(void)h;return ESP_OK;}
int64_t esp_timer_get_time(void) {return 10000000;}
int64_t time_manager_get_epoch(void) {return 1700000000;}
const char *impact_state_to_string(package_state_t s) {(void)s;return "IMPACT";}
static esp_err_t add(void) {return event_log_add(PACKAGE_IMPACT,IMPACT_LEVEL_STRONG,8,0,0,9000,30,false);}
int main(void) {
    assert(event_log_init()==ESP_OK);
    for(int i=0;i<EVENT_LOG_MAX_EVENTS;i++) assert(add()==ESP_OK);
    event_record_t e;
    assert(add()==ESP_ERR_NO_MEM && event_log_rejected_count()==1);
    assert(event_log_unsynced_count()==EVENT_LOG_MAX_EVENTS);
    assert(event_log_get_first_unsynced(&e) && e.id==1);
    assert(e.timestamp==1699999999 && e.duration_ms==30 && e.measurement_version==1);
    fail_save=1;assert(event_log_mark_synced(1)==ESP_FAIL);
    assert(event_log_get_first_unsynced(&e) && e.id==1);
    assert(event_log_clear()==ESP_FAIL);
    assert(event_log_get_first_unsynced(&e) && e.id==1);
    fail_save=0;assert(event_log_mark_synced(1)==ESP_OK);assert(add()==ESP_OK);
    assert(event_log_get_first_unsynced(&e) && e.id==2);
    assert(event_log_init()==ESP_OK);assert(event_log_unsynced_count()==EVENT_LOG_MAX_EVENTS);
    // Migrate a wrapped V2 ring in chronological order, retaining IDs, timestamps and unsynced flags.
    event_store_v2_t legacy={.magic=EVENT_STORE_MAGIC,.version=2,.next_id=101,.count=50,.head=10};
    for(int i=0;i<50;i++) {int pos=(10+i)%50;legacy.events[pos].id=51+i;legacy.events[pos].timestamp=123;}
    memcpy(blob,&legacy,sizeof(legacy));blob_size=sizeof(legacy);
    assert(event_log_init()==ESP_OK);
    assert(event_log_count()==50 && event_log_get_first_unsynced(&e) && e.id==51);
    assert(e.timestamp==123 && e.duration_ms==0 && !e.synced && !e.measurement_version);
    assert(event_log_clear()==ESP_OK);assert(add()==ESP_OK);
    assert(event_log_get_first_unsynced(&e) && e.id==101);
    puts("PASS: overflow preserves pending events, failed writes remain pending, reboot recovery, V2 migration and monotonic IDs");
}
