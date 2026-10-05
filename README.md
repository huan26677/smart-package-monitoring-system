# Smart Package Monitoring System

Hệ thống IoT giám sát va đập, trạng thái vận chuyển và vị trí tương đối của kiện hàng.

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

```
smart-package/esp32-001/telemetry
smart-package/esp32-001/event
smart-package/esp32-001/location-scan
```

## Main REST APIs

```
GET /api/health
GET /api/devices
GET /api/devices/{deviceId}/dashboard
GET /api/devices/{deviceId}/telemetry
GET /api/devices/{deviceId}/events
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
