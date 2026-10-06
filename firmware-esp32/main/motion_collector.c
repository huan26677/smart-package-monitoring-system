#include "motion_collector.h"
#include "mqtt_manager.h"
#include "time_manager.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define SAMPLE_COUNT 200
#define PAYLOAD_SIZE 24000
typedef struct {uint64_t ms;float axes[6];} sample_t;
typedef struct {sample_t samples[SAMPLE_COUNT];char session[37];unsigned sequence;unsigned window_id;unsigned generation;int64_t timestamp;int rule;} frame_t;
typedef struct {char session[37];unsigned sequence;} receipt_t;
static QueueHandle_t frames,live_frames,receipts;
static SemaphoreHandle_t control_mutex;
static char session[37],boot_id[33];
static unsigned remaining,next_sequence,generation;
static frame_t building;
static unsigned fill,building_generation,window_id;
static const char *TAG="MOTION_AI";
static bool valid_session(const char *s) {
    if(!s || strlen(s)!=36) return false;
    for(int i=0;i<36;i++) if(!((s[i]>='a'&&s[i]<='f')||(s[i]>='0'&&s[i]<='9')||s[i]=='-')) return false;
    return true;
}
void motion_collector_control(const cJSON *root) {
    if(!control_mutex) return;
    const cJSON *action=cJSON_GetObjectItemCaseSensitive(root,"action");
    const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"sessionId");
    const cJSON *count=cJSON_GetObjectItemCaseSensitive(root,"windows");
    if(!cJSON_IsString(action)||!cJSON_IsString(id)||!valid_session(id->valuestring)) return;
    xSemaphoreTake(control_mutex,portMAX_DELAY);
    if(!strcmp(action->valuestring,"START") && cJSON_IsNumber(count) && count->valuedouble>=5
            && count->valuedouble<=60 && floor(count->valuedouble)==count->valuedouble && strcmp(session,id->valuestring)) {
        strcpy(session,id->valuestring);remaining=(unsigned)count->valuedouble;next_sequence=1;generation++;
        ESP_LOGI(TAG,"Bat dau thu %u doan cam bien",remaining);
    } else if(!strcmp(action->valuestring,"STOP")&&!strcmp(session,id->valuestring)) {remaining=0;generation++;}
    xSemaphoreGive(control_mutex);
}
void motion_collector_receipt(const cJSON *root) {
    if(!receipts) return;
    const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"sessionId");
    const cJSON *seq=cJSON_GetObjectItemCaseSensitive(root,"sessionWindow");
    const cJSON *status=cJSON_GetObjectItemCaseSensitive(root,"status");
    if(cJSON_IsString(id)&&valid_session(id->valuestring)&&cJSON_IsString(status)&&!strcmp(status->valuestring,"SAVED")
            && cJSON_IsNumber(seq)&&seq->valuedouble>=1&&seq->valuedouble<=60&&floor(seq->valuedouble)==seq->valuedouble) {
        receipt_t r={.sequence=(unsigned)seq->valuedouble};strcpy(r.session,id->valuestring);xQueueSend(receipts,&r,0);
    }
}
void motion_collector_discard_partial(void) {fill=0;}
void motion_collector_add(const mpu6050_data_t *sensor,const impact_result_t *result,uint64_t sample_ms) {
    if(!frames || !result->calibrated) return;
    xSemaphoreTake(control_mutex,portMAX_DELAY);
    if(building_generation!=generation) {fill=0;building_generation=generation;}
    if(fill==0) {
        memset(&building,0,sizeof(building));
        building.generation=building_generation;
        if(remaining) {strcpy(building.session,session);building.sequence=next_sequence;}
        building.timestamp=time_manager_get_epoch();
    }
    xSemaphoreGive(control_mutex);
    sample_t *sample=&building.samples[fill];sample->ms=sample_ms;
    if(fill && sample->ms<=building.samples[fill-1].ms) return;
    float values[]={sensor->ax,sensor->ay,sensor->az,sensor->gx,sensor->gy,sensor->gz};
    for(unsigned i=0;i<6;i++) {if(!isfinite(values[i])) {fill=0;return;}sample->axes[i]=values[i];}
    if(result->state==PACKAGE_IMPACT||result->state==PACKAGE_DROP) building.rule=2;
    else if(result->state==PACKAGE_VIBRATION&&building.rule<1) building.rule=1;
    if(++fill<SAMPLE_COUNT) return;
    building.window_id=++window_id;
    if(!building.session[0]) xQueueOverwrite(live_frames,&building);
    else if(xQueueSend(frames,&building,0)!=pdTRUE) ESP_LOGW(TAG,"Hang doi day: bo qua doan %u; dashboard se bao thieu doan",building.window_id);
    xSemaphoreTake(control_mutex,portMAX_DELAY);
    if(building.session[0] && building_generation==generation && remaining) {remaining--;next_sequence++;}
    xSemaphoreGive(control_mutex);
    fill=0;
}
static int format_frame(const frame_t *f,char *payload,size_t capacity) {
    static const char *labels[]={"NORMAL","VIBRATION","IMPACT"};
    int used=snprintf(payload,capacity,"{\"deviceId\":\"esp32-001\",\"schemaVersion\":1,\"bootId\":\"%s\",\"windowId\":%u,\"sessionId\":\"%s\",\"sessionWindow\":%u,\"timestamp\":%lld,\"ruleLabel\":\"%s\",\"samples\":[",
            boot_id,f->window_id,f->session,f->sequence,(long long)f->timestamp,labels[f->rule]);
    if(used<0||(size_t)used>=capacity) return -1;
    for(unsigned i=0;i<SAMPLE_COUNT;i++) {
        const sample_t *s=&f->samples[i];
        int n=snprintf(payload+used,capacity-used,"%s[%llu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f]",i?",":"",(unsigned long long)s->ms,
                s->axes[0],s->axes[1],s->axes[2],s->axes[3],s->axes[4],s->axes[5]);
        if(n<0||(size_t)n>=capacity-used) return -1;
        used+=n;
    }
    int n=snprintf(payload+used,capacity-used,"]}");return n==2?used+n:-1;
}
static bool capture_is_current(const frame_t *frame) {
    xSemaphoreTake(control_mutex,portMAX_DELAY);
    bool current=frame->generation==generation && !strcmp(frame->session,session);
    xSemaphoreGive(control_mutex);
    return current;
}
static void sender_task(void *arg) {
    (void)arg;
    static frame_t frame;
    static char payload[PAYLOAD_SIZE];
    receipt_t receipt;
    while(1) {
        if(xQueueReceive(frames,&frame,pdMS_TO_TICKS(100))!=pdTRUE
                && xQueueReceive(live_frames,&frame,0)!=pdTRUE) continue;
        if(frame.session[0] && !capture_is_current(&frame)) continue;
        int length=format_frame(&frame,payload,sizeof(payload));
        if(length<0) {ESP_LOGE(TAG,"Doan cam bien vuot gioi han JSON");continue;}
        if(!frame.session[0]) {if(mqtt_manager_is_connected()) mqtt_manager_publish_motion(payload,length,false);continue;}
        // Captured data gets a DB receipt; the bounded RAM queue is not a power-loss journal.
        int64_t end=esp_timer_get_time()+30000000LL,next_send=0;bool confirmed=false;
        while(esp_timer_get_time()<end && !confirmed && capture_is_current(&frame)) {
            if(mqtt_manager_is_connected() && esp_timer_get_time()>=next_send) {
                mqtt_manager_publish_motion(payload,length,true);next_send=esp_timer_get_time()+10000000LL;
            }
            if(xQueueReceive(receipts,&receipt,pdMS_TO_TICKS(100))==pdTRUE)
                confirmed=!strcmp(receipt.session,frame.session)&&receipt.sequence==frame.sequence;
        }
        if(!confirmed && capture_is_current(&frame)) ESP_LOGW(TAG,"Doan thu #%u chua duoc database xac nhan",frame.sequence);
    }
}
esp_err_t motion_collector_init(void) {
    control_mutex=xSemaphoreCreateMutex();frames=xQueueCreate(4,sizeof(frame_t));live_frames=xQueueCreate(1,sizeof(frame_t));receipts=xQueueCreate(8,sizeof(receipt_t));
    if(!control_mutex||!frames||!live_frames||!receipts) return ESP_ERR_NO_MEM;
    snprintf(boot_id,sizeof(boot_id),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());
#if CONFIG_FREERTOS_UNICORE
    const BaseType_t sender_core=0;
#else
    const BaseType_t sender_core=1;
#endif
    return xTaskCreatePinnedToCore(sender_task,"motion_sender",4096,NULL,1,NULL,sender_core)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}
