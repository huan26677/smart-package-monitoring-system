# Backend Testing

## Unit Tests

Backend sử dụng JUnit 5 và Mockito.

Các unit test hiện tại không yêu cầu:

- PostgreSQL
- EMQX
- ESP32
- Docker

## Run Tests

Từ thư mục:

`smart_package_backend`

chạy:

```powershell
.\mvnw.cmd test
```

Kết quả mong đợi:

- Failures: 0
- Errors: 0
- BUILD SUCCESS

### MQTT recovery tests

`MqttSubscriberTests` kiểm tra kết nối thất bại lần đầu, kết nối lại và đăng ký lại topic sau khi mất kết nối, cùng việc thử lại khi đăng ký topic thất bại. Các test dùng MQTT client giả lập, không cần broker thật.

Kiểm tra vận hành với broker thật:

1. Chạy các dịch vụ theo [RUNNING.md](RUNNING.md).
2. Xác nhận `/api/health/mqtt` trả HTTP 200 và thiết bị ESP32 cập nhật telemetry.
3. Dừng EMQX bằng `docker compose stop emqx`: MQTT health phải trả HTTP 503, API database vẫn trả HTTP 200.
4. Khởi động EMQX bằng `docker compose start emqx`: backend phải tự kết nối lại, đăng ký đủ 3 topic và nhận dữ liệu mới.
5. Để kiểm tra lỗi lần kết nối đầu, dừng EMQX, khởi động lại backend, sau đó bật EMQX; backend phải phục hồi mà không cần khởi động lại lần nữa.

## Test Coverage

### WifiAnchorServiceTests
Kiểm tra:

- Chuẩn hóa BSSID thành chữ hoa
- Radius mặc định 30 mét
- Chọn Wi-Fi Anchor có RSSI mạnh nhất
- Trạng thái `ANCHOR_MATCHED`
- Trạng thái `NO_ANCHOR`
- HTTP 404 khi xóa Anchor không tồn tại

### WifiLocationRequestValidationTests
Kiểm tra:

- BSSID hợp lệ
- BSSID sai định dạng
- Tên Anchor bắt buộc
- Latitude trong khoảng -90 đến 90
- Longitude trong khoảng -180 đến 180
- Radius phải lớn hơn 0

### DeviceDataServiceTests
Kiểm tra:

- Thiết bị mới nhận dữ liệu được xác định ONLINE
- Dashboard chấp nhận trường telemetry/event/location rỗng
- Location mới nhất trả `NO_ANCHOR` thay vì vị trí cũ
