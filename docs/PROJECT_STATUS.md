# Smart Package Monitoring System - Project Status

## Current Stage

Giai đoạn 8A - Web Dashboard

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
- REST API danh sách Device
- REST API chi tiết Device
- REST API telemetry mới nhất
- REST API lịch sử telemetry
- REST API lịch sử package event
- REST API lịch sử location scan
- REST API trả Wi-Fi access points theo location scan
- Wi-Fi Anchor geolocation hoàn toàn local
- Không phụ thuộc Google Geolocation API
- Lưu latitude / longitude / bán kính Anchor vào location scan
- Lưu Anchor name, BSSID và RSSI đã match
- REST API quản lý Wi-Fi Anchor `/api/wifi-locations`
- REST API vị trí mới nhất `/api/devices/{deviceId}/location/latest`
- Dashboard API `/api/devices/{deviceId}/dashboard`
- Dashboard tổng hợp telemetry mới nhất
- Dashboard tổng hợp event mới nhất
- Dashboard tổng hợp Wi-Fi Anchor location mới nhất
- Xác định trạng thái thiết bị ONLINE/OFFLINE theo `lastSeenAt`
- Global REST API exception handler
- Chuẩn hóa JSON error response
- Validation dữ liệu Wi-Fi Anchor
- HTTP 400 cho request không hợp lệ
- HTTP 404 cho resource không tồn tại
- HTTP 204 khi xóa Wi-Fi Anchor thành công
- CORS cho Web frontend trong môi trường local
- Unit test bằng JUnit 5
- Mockito cho Service test
- Test Wi-Fi Anchor matching
- Test chọn Anchor theo RSSI mạnh nhất
- Test Wi-Fi Anchor validation
- Test Dashboard API service logic
- Test chống trả vị trí Anchor cũ
- Unit test không phụ thuộc PostgreSQL / EMQX / ESP32

## Database

PostgreSQL chạy bằng Docker.

Đã hoàn thành:
- JPA Entity `wifi_locations`
- PostgreSQL lưu danh sách Wi-Fi Anchor
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
- Lưu MQTT telemetry vào PostgreSQL
- Lưu MQTT event vào PostgreSQL
- Lưu MQTT location scan vào PostgreSQL
- Lưu Wi-Fi access points theo từng location scan
- Tự động tạo/cập nhật Device theo `deviceId`
- Chống lưu trùng event theo `(device_id, event_id)`

## Web Dashboard

Technology:

- React
- Vite
- JavaScript

Đã hoàn thành:

- Khởi tạo React Web Dashboard
- Cấu hình Spring Boot REST API base URL
- Đọc danh sách Device từ Backend
- Đọc Dashboard API theo Device
- Hiển thị ONLINE / OFFLINE
- Hiển thị Total G
- Hiển thị Angle
- Hiển thị Vibration
- Hiển thị Wi-Fi RSSI
- Hiển thị Event mới nhất
- Hiển thị Wi-Fi Anchor location
- Xử lý loading state
- Xử lý REST API error
- Production build bằng Vite

## Mobile

Chưa làm.

Dự kiến:

Flutter

## Next Step

Giai đoạn 8B - Hoàn thiện giao diện Web Dashboard, tự động cập nhật dữ liệu và biểu đồ telemetry.