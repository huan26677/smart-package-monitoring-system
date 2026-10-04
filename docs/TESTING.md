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
