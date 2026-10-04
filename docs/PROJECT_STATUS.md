# Smart Package Monitoring System - Project Status

## Current Stage

Giai đoạn 7C - Bắt đầu Spring Boot Backend

## ESP32 Firmware

Đã hoàn thành:

- ESP32-S3 + MPU6050/MPU6500-compatible
- I2C
- Calibration tư thế NORMAL
- Phát hiện:
  - Vibration
  - Tilt
  - Flip
  - Free Fall
  - Impact
  - Drop
- LCD1602
- Active Buzzer
- Event Log NVS
- Event Store V2
- MQTT telemetry
- MQTT event
- MQTT offline synchronization
- Wi-Fi Setup Portal
- Wi-Fi failover
- NTP timestamp
- Wi-Fi BSSID/RSSI scan
- MQTT location-scan

## MQTT

Broker:

EMQX

Topics:

- smart-package/esp32-001/telemetry
- smart-package/esp32-001/event
- smart-package/esp32-001/location-scan

## Backend

Đang bắt đầu từ con số 0.

Technology:

- Java 17
- Spring Boot
- Maven

## Database

Chưa làm.

Dự kiến:

PostgreSQL

## Mobile

Chưa làm.

Dự kiến:

Flutter

## Next Step

Tạo Spring Boot Backend và kiểm tra REST API /api/health.