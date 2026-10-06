# Smart Package Monitoring System

Hệ thống IoT giám sát va đập, trạng thái vận chuyển và vị trí tương đối của kiện hàng.

Đã bổ sung thu dữ liệu cảm biến, gắn nhãn và phân loại AI ba nhóm. Xem [hướng dẫn AI bằng tiếng Việt](docs/AI_GUIDE.md). Mô hình chỉ được nạp sau khi thu đủ dữ liệu thật và đánh giá theo nhóm buổi thử.

## Architecture

```text
ESP32-S3
   |
   | MQTT
   v
EMQX
   |
   v
Spring Boot
   |
   +------ PostgreSQL
   |
   v
React Web Dashboard
   |
   v
Nginx
   |
   v
Cloudflare Tunnel
   |
   v
Internet / HTTPS
```

## Hardware

- ESP32-S3
- MPU6050 / MPU6500-compatible
- LCD1602
- Active buzzer
- Wi-Fi

## Package States

- NORMAL
- VIBRATION
- TILT
- FLIP
- FREE_FALL
- IMPACT
- DROP

## Technology

### Firmware

- ESP-IDF
- C
- NVS
- MQTT

### Backend

- Java 17
- Spring Boot
- Spring Data JPA
- PostgreSQL
- Eclipse Paho MQTT

### Web

- React
- Vite
- Recharts
- Leaflet
- OpenStreetMap
- Nginx

### Infrastructure

- Docker
- Docker Compose
- EMQX
- PostgreSQL
- Cloudflare Tunnel

## Main MQTT Topics

Đổi Wi-Fi và MQTT khi mang ESP32 sang nơi khác: xem [hướng dẫn cấu hình mạng](docs/MOBILE_NETWORK.md). Thiết bị dùng WSS qua Cloudflare và trang cấu hình tiếng Việt, không cần nạp lại mỗi lần đổi mạng.

```
smart-package/esp32-001/telemetry
smart-package/esp32-001/event
smart-package/esp32-001/location-scan
smart-package/esp32-001/event-ack
```

## Main REST APIs

```
GET /api/health
GET /api/devices
GET /api/devices/{deviceId}/dashboard
GET /api/devices/{deviceId}/telemetry
GET /api/devices/{deviceId}/events
GET /api/devices/{deviceId}/event-history
GET /api/devices/{deviceId}/event-summary
GET /api/devices/{deviceId}/events.csv
GET /api/devices/{deviceId}/location/latest
GET /api/devices/{deviceId}/location-scans

GET    /api/wifi-locations
POST   /api/wifi-locations
DELETE /api/wifi-locations
```

## Start Production System
Create local `.env`:

```
POSTGRES_DB=smart_package
POSTGRES_USER=smartpackage
POSTGRES_PASSWORD=YOUR_PASSWORD
DASHBOARD_ADMIN_USERNAME=admin
DASHBOARD_ADMIN_PASSWORD=YOUR_STRONG_PASSWORD_AT_LEAST_16_CHARACTERS

CLOUDFLARE_TUNNEL_TOKEN=YOUR_TOKEN
```

Run:

```
docker compose up -d --build
```

Check:

```
docker compose ps
```

Logs:

```
docker compose logs backend --tail=100
docker compose logs cloudflared --tail=100
```

## Local Web

```
http://localhost:8088
```

Dashboard và API dữ liệu yêu cầu đăng nhập. Tài khoản và mật khẩu lấy từ `.env`; không đưa file này lên Git. Hướng dẫn vận hành: [docs/RUNNING.md](docs/RUNNING.md).

Firmware gom các mẫu của một va đập để lưu đỉnh gia tốc, thời lượng và dấu hiệu chạm giới hạn đo. Sự kiện được lưu trên ESP32, gửi lại đến khi backend xác nhận đã lưu vào PostgreSQL; QoS 1 của broker không thay thế xác nhận này. Dashboard có bộ lọc, phân trang, thống kê và xuất CSV.

Ngưỡng phân loại hiện dùng cho thử nghiệm. Kế hoạch đo thực tế và biểu mẫu báo cáo: [docs/EXPERIMENTS.md](docs/EXPERIMENTS.md).

## Public Web
The production Web Dashboard is published through Cloudflare Tunnel using a custom HTTPS domain.

## Location Model
The system does not use GPS.

Location is estimated relatively using registered Wi-Fi Anchors:

```
Wi-Fi BSSID + RSSI
        |
        v
Known Wi-Fi Anchor
        |
        v
Relative package location
```

`ANCHOR_MATCHED` means a known Anchor was detected.

`NO_ANCHOR` means the current scan does not contain a registered Anchor.

## Project Status
See:

```
docs/PROJECT_STATUS.md
```
