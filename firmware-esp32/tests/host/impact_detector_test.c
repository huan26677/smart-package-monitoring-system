#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "impact_detector.h"
static uint32_t now;
static impact_result_t result;
static void sample(float g) {
    mpu6050_data_t sensor = {.az=g,.total_g=g};
    impact_detector_update(&sensor,&result,now);
    now += 10;
}
static void baseline(void) {
    now = 0; impact_detector_init();
    for(int i=0;i<201;i++) sample(1);
    assert(result.calibrated);
}
int main(void) {
    baseline();
    sample(2.6f); assert(!result.new_event);
    sample(8); assert(!result.new_event);
    sample(3); assert(!result.new_event);
    int impacts=0;
    for(int i=0;i<15;i++) {
        sample(1);
        if(result.new_event && result.event_type==PACKAGE_IMPACT) {
            impacts++; assert(fabsf(result.event_peak_g-8)<.001);
            assert(result.event_level==IMPACT_LEVEL_STRONG);
            assert(result.event_duration_ms==30);
        }
    }
    assert(impacts==1);
    baseline();
    for(int i=0;i<6;i++) sample(.1f);
    sample(4.5f);
    for(int i=0;i<8;i++) sample(1);
    assert(result.new_event && result.event_type==PACKAGE_DROP);
    assert(result.event_level==IMPACT_LEVEL_MEDIUM);
    baseline(); sample(16);
    for(int i=0;i<8;i++) sample(1);
    assert(result.new_event && result.event_saturated);
    baseline();
    for(int i=0;i<100;i++) {sample(1);assert(result.state==PACKAGE_NORMAL && !result.new_event);}
    // Unsigned elapsed times remain correct across the 32-bit uptime rollover.
    now=UINT32_MAX-30;sample(3);
    for(int i=0;i<8;i++) sample(1);
    assert(result.new_event && result.event_type==PACKAGE_IMPACT);
    puts("PASS: peak aggregation, strongest level, single episode, drop, saturation, baseline and uptime rollover");
}
