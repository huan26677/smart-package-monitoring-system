# Smart Package Monitoring System - Project Status

## Current Stage

Giai đoạn 9 - Hoàn thiện, triển khai và kiểm thử hệ thống

Phần AI: đã có thu chuỗi 200 mẫu/đoạn, gắn nhãn thủ công, xuất dữ liệu, huấn luyện Random Forest trên máy tính và suy luận Java. Mô hình chưa huấn luyện bằng dữ liệu kiện hàng thật; cần người dùng thực hiện các buổi thử theo [AI_GUIDE.md](AI_GUIDE.md). Các cảnh báo theo ngưỡng tiếp tục hoạt động độc lập.

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
- Event Store V3 (128 bản ghi, chuyển lịch sử V1/V2)
- MQTT telemetry
- MQTT event
- MQTT offline synchronization
- Wi-Fi Setup Portal
- Wi-Fi failover
- Đổi Wi-Fi/MQTT qua trang cấu hình tiếng Việt, lưu riêng tài khoản và mật khẩu MQTT trong NVS
- MQTT WSS qua tên miền Cloudflare, xác thực chứng chỉ và tài khoản
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
- Auto refresh Dashboard mỗi 3 giây
- Background refresh không làm mất dữ liệu hiện tại
- Đọc 60 telemetry gần nhất
- Biểu đồ telemetry bằng Recharts
- Hiển thị lịch sử Total G
- Hiển thị lịch sử Angle
- Hiển thị lịch sử Vibration
- Responsive layout cho màn hình nhỏ
- Tách biểu đồ Total G / Angle / Vibration thành 3 biểu đồ riêng
- Hiển thị timestamp telemetry trên biểu đồ
- REST API lịch sử Event được tích hợp vào Web
- Bảng 20 package event gần nhất
- Hiển thị Event Type / Level / G / Angle / Vibration
- Banner cảnh báo theo trạng thái NORMAL / VIBRATION / TILT / FLIP / FREE_FALL / IMPACT / DROP
- Hiển thị cảnh báo OFFLINE
- Auto refresh cả telemetry và event history
- Bản đồ vị trí bằng React Leaflet
- OpenStreetMap base map
- Hiển thị toàn bộ Wi-Fi Anchor trên bản đồ
- Hiển thị vị trí tương đối hiện tại của kiện hàng
- Quản lý danh sách Wi-Fi Anchor trên Web
- Thêm / cập nhật Anchor bằng REST API
- Xóa Anchor bằng REST API
- Chọn BSSID từ Wi-Fi scan gần nhất của ESP32
- Hỗ trợ lấy latitude / longitude từ Browser Geolocation
- Không hiển thị vị trí giả khi trạng thái là `NO_ANCHOR`
- Đọc 50 location scan gần nhất
- Hiển thị bảng lịch sử vị trí
- Hiển thị trạng thái `ANCHOR_MATCHED` / `NO_ANCHOR`
- Hiển thị BSSID và RSSI của Anchor đã match
- Hiển thị latitude / longitude lịch sử
- Vẽ lịch sử di chuyển tương đối trên OpenStreetMap
- Loại bỏ các điểm Anchor liên tiếp bị trùng trên đường lịch sử
- Không đưa `NO_ANCHOR` vào tuyến vị trí
- Responsive Web Dashboard cho desktop và điện thoại
- Production build bằng Docker
- Nginx phục vụ React production
- Nginx reverse proxy `/api` sang Spring Boot
- Cloudflare Tunnel chạy bằng Docker
- Public Web Dashboard qua tên miền riêng
- HTTPS public qua Cloudflare
- Không cần port forwarding
- Không cần public IP tĩnh

## Deployment

Đã hoàn thành:

- Docker Compose quản lý toàn bộ infrastructure
- EMQX health check
- PostgreSQL health check
- Spring Boot health check
- Backend chỉ khởi động sau PostgreSQL và EMQX
- Web chỉ khởi động sau Backend
- Cloudflare Tunnel tự động restart
- Public HTTPS domain
- PostgreSQL chỉ expose localhost
- Backend chỉ expose localhost
- Web production chỉ expose localhost
- MQTT port 1883 phục vụ LAN; WSS `mqtt.huan2k5.id.vn:443/mqtt` phục vụ ESP32 ngoài LAN

## Cập nhật độ tin cậy và dashboard

- Gom các mẫu va đập thành đợt; lưu đỉnh g, mức mạnh nhất, thời lượng quan sát và dấu hiệu chạm giới hạn đo; LCD và buzzer dùng cùng kết quả.
- Xác nhận sự kiện sau khi backend lưu PostgreSQL; gửi lại khi chưa có xác nhận, chống trùng `(device_id, event_id)`.
- Tiếp tục giám sát và kết nối lại Wi-Fi; mở thêm AP cấu hình sau khoảng 60 giây chưa có IP. Giữ BOOT 3 giây rồi thả cũng mở cấu hình, không xóa lịch sử.
- Không ghi đè bản NVS chưa đồng bộ khi đầy; có bộ đếm bản ghi mới bị từ chối và cảnh báo trên dashboard.
- Đăng nhập dashboard/API, cookie phiên HttpOnly, SameSite=Lax, CSRF cho thao tác ghi; thông tin tài khoản từ `.env`.
- Tách giờ xảy ra và giờ nhận; bộ lọc loại/mức/thời gian, phân trang, thống kê toàn bộ kết quả lọc và xuất CSV.
- Cảnh báo va đập/rơi được giữ đến khi người dùng xác nhận đã xem trong trình duyệt.
- 16 unit test backend; kiểm thử C với dữ liệu cảm biến/NVS mô phỏng. Xem [TESTING.md](TESTING.md).

## Next Step

Thực hiện đo trên kiện mẫu, đối chiếu nhãn quan sát và đánh giá ngưỡng theo [EXPERIMENTS.md](EXPERIMENTS.md). Các ngưỡng gia tốc hiện tại là ngưỡng thử nghiệm; chưa có kết quả vật lý chứng minh độ chính xác phát hiện hoặc mức hư hại của hàng.
