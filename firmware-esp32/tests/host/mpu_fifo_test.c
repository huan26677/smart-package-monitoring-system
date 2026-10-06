#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "mpu6050.h"

static uint8_t who = 0x70, before_status, after_status;
static unsigned fifo_count, status_reads;
static bool fail_data, slow_count;
static int64_t now_us;
static uint8_t packet[14], configured[128];
int64_t esp_timer_get_time(void) {return now_us;}
void vTaskDelay(unsigned ticks) {assert(ticks > 0); now_us += ticks * 10000;}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *cfg, i2c_master_dev_handle_t *dev) {
    (void)bus; assert(cfg->scl_speed_hz == 400000); *dev = (void *)1; return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev, const uint8_t *data, size_t n, int timeout) {
    (void)dev; (void)timeout; assert(n == 2); configured[data[0]] = data[1]; return ESP_OK;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t dev, const uint8_t *reg, size_t n, uint8_t *data, size_t length, int timeout) {
    (void)dev; (void)timeout; assert(n == 1);
    switch (*reg) {
        case 0x75: data[0] = who; break;
        case 0x3A: data[0] = status_reads++ ? after_status : before_status; break;
        case 0x72: assert(length == 2); data[0] = fifo_count >> 8; data[1] = fifo_count; if(slow_count) now_us += 30000; break;
        case 0x74:
            if(fail_data) return ESP_FAIL;
            assert(length <= fifo_count && length % 14 == 0);
            for(size_t i=0;i<length;i++) data[i] = packet[i%14];
            fifo_count -= length;
            break;
        default: assert(0);
    }
    return ESP_OK;
}
static void scenario(unsigned bytes) {
    assert(mpu6050_fifo_start() == ESP_OK);
    fifo_count = bytes; status_reads = 0; before_status = after_status = 0;
    fail_data = slow_count = false; now_us = 1000000;
}
int main(void) {
    assert(mpu6050_init(NULL) == ESP_OK);
    assert(configured[0x19] == 9);
    assert(mpu6050_fifo_start() == ESP_OK);
    assert(configured[0x23] == 0xF8 && configured[0x6A] == 0x40 && configured[0x38] == 0x10);
    packet[4] = 0x08; // Z = 2048 LSB = 1g
    packet[8] = 0xFF; packet[9] = 0xBF; // GX = -65 LSB
    mpu6050_sample_t rows[MPU6050_FIFO_MAX_SAMPLES]; size_t count;
    scenario(0); assert(mpu6050_fifo_read(rows,73,&count)==ESP_OK && count==0);
    scenario(3*14+3); assert(mpu6050_fifo_read(rows,73,&count)==ESP_OK && count==3 && fifo_count==3);
    assert(rows[0].estimated_ms==960 && rows[1].estimated_ms==970 && rows[2].estimated_ms==980);
    assert(fabsf(rows[0].data.az-1)<1e-6 && fabsf(rows[0].data.total_g-1)<1e-6);
    /* A slightly faster sensor sometimes supplies 2 packets in a 10 ms poll.
     * The old per-batch anchor made the first timestamp duplicate the last. */
    now_us += 10000; fifo_count = 28; status_reads = 0;
    assert(mpu6050_fifo_read(rows,73,&count)==ESP_OK && count==2);
    assert(rows[0].estimated_ms==989 && rows[1].estimated_ms==999);
    now_us += 10000; fifo_count = 14; status_reads = 0;
    assert(mpu6050_fifo_read(rows,73,&count)==ESP_OK && count==1 && rows[0].estimated_ms==1008);
    now_us += 10000; fifo_count = 140; status_reads = 0;
    assert(mpu6050_fifo_read(rows,73,&count)==ESP_ERR_INVALID_STATE && count==0);
    assert(rows[0].data.gx<0 && fabsf(rows[0].data.gx+65.0f/65.5f)<1e-6);
    scenario(14); before_status=0x10; assert(mpu6050_fifo_read(rows,73,&count)==ESP_ERR_INVALID_STATE && count==0);
    scenario(14); after_status=0x10; assert(mpu6050_fifo_read(rows,73,&count)==ESP_ERR_INVALID_STATE && count==0);
    scenario(512); assert(mpu6050_fifo_read(rows,73,&count)==ESP_ERR_INVALID_STATE && count==0);
    scenario(28); assert(mpu6050_fifo_read(rows,1,&count)==ESP_ERR_INVALID_SIZE && count==0);
    scenario(14); fail_data=true; assert(mpu6050_fifo_read(rows,73,&count)==ESP_FAIL && count==0);
    scenario(14); slow_count=true; assert(mpu6050_fifo_read(rows,73,&count)==ESP_ERR_INVALID_STATE && count==0);
    assert(mpu6050_fifo_read(NULL,73,&count)==ESP_ERR_INVALID_ARG);
    who=0x68; assert(mpu6050_init(NULL)==ESP_OK);
    scenario(72*14); assert(mpu6050_fifo_read(rows,73,&count)==ESP_OK && count==72);
    puts("FIFO driver: decoding, timestamps, overflow and uncertain-read checks passed");
}
