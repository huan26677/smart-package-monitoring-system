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

EMQX chạy bằng Docker.

Infrastructure:

- EMQX MQTT broker
- MQTT TCP port 1883
- WebSocket port 8083
- EMQX Dashboard port 18083
- EMQX được quản lý chung bằng root Docker Compose

Topics:

- smart-package/esp32-001/telemetry
- smart-package/esp32-001/event
- smart-package/esp32-001/location-scan

## Backend

Spring Boot backend đã khởi tạo và chạy thành công.

Technology:

- Java 17
- Spring Boot 4.1.1
- Maven
- Eclipse Paho MQTT

Đã hoàn thành:

- REST API `/api/health`
- Kết nối EMQX
- Subscribe MQTT telemetry
- Subscribe MQTT event
- Subscribe MQTT location-scan
- Parse MQTT telemetry JSON thành Java DTO
- Parse MQTT event JSON thành Java DTO
- Parse MQTT location-scan JSON thành Java DTO
- Tách MQTT parsing sang service layer

## Database

PostgreSQL chạy bằng Docker.

Đã hoàn thành:

- PostgreSQL container
- Persistent Docker volume
- Spring Boot kết nối PostgreSQL
- JDBC database health check
- REST API `/api/health/database`
- JPA Entity `devices`
- JPA Entity `telemetry`
- JPA Entity `package_events`
- JPA Entity `location_scans`
- JPA Entity `wifi_access_points`
- Spring Data JPA Repository
- Quan hệ khóa ngoại giữa Device, Telemetry, Event và Location Scan
- Unique constraint `(device_id, event_id)` chống trùng event

## Mobile

Chưa làm.

Dự kiến:

Flutter

## Next Step

Giai đoạn 7C-5 - Lưu MQTT Telemetry, Event và Location Scan vào PostgreSQL.