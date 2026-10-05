# Chạy hệ thống trên máy

Yêu cầu: Docker Desktop đang chạy và file `.env` ở thư mục gốc đã có thông tin PostgreSQL.

Từ thư mục gốc dự án:

```powershell
docker compose up -d --build postgres emqx backend web
docker compose ps
```

Mở dashboard tại http://localhost:8088. Backend tại http://localhost:8080.
Web và API hiện chỉ được mở trên máy chạy Docker. ESP32 kết nối MQTT qua địa chỉ IP LAN của máy đó, cổng 1883.

Compose đợi EMQX sẵn sàng rồi mới chạy backend; đợi backend kết nối được PostgreSQL và đăng ký các topic MQTT rồi mới chạy web. Lần khởi động đầu có thể mất vài phút để build và khởi tạo dịch vụ.

## Kiểm tra tình trạng

```powershell
Invoke-RestMethod http://localhost:8088/api/health/database
Invoke-RestMethod http://localhost:8088/api/health/mqtt
docker compose logs --tail 100 backend
```

Database và MQTT đều phải trả `status: OK`. `/api/health/mqtt` trả HTTP 503 khi backend chưa kết nối hoặc chưa đăng ký topic MQTT. `/api/health` kiểm tra tiến trình HTTP, không thay thế kiểm tra database và MQTT.

Backend tự thử lại kết nối và đăng ký topic mỗi 5 giây, kể cả khi broker chưa chạy lúc backend khởi động. Khi broker mất kết nối hoặc khởi động lại, backend tự kết nối lại. Có thể cấu hình thời gian giữa các lần thử bằng biến môi trường `MQTT_RETRY_INTERVAL_MS` khi chạy backend.

Dashboard tự cập nhật dữ liệu và danh sách thiết bị mỗi 3 giây. Nếu ESP32 đang gửi dữ liệu, thiết bị phải hiển thị `ONLINE` và thời điểm nhận telemetry phải thay đổi.

Web tự phân giải lại địa chỉ backend bằng DNS của Docker. Khi backend khởi động lại hoặc được tạo lại với IP mới, web phục hồi kết nối API mà không cần khởi động lại Nginx.

`NO_ANCHOR` nghĩa là chưa có BSSID trong lần quét khớp với Wi-Fi Anchor đã đăng ký. Để hiển thị vị trí, chọn BSSID thật từ danh sách Wi-Fi ESP32 vừa quét và lưu tên vị trí cùng tọa độ thật trong phần Wi-Fi Anchor Management.

## Dừng hệ thống

```powershell
docker compose stop web backend emqx postgres
```

Dữ liệu PostgreSQL được lưu trong volume và giữ lại khi dừng dịch vụ.

## Cloudflare Tunnel

Để sử dụng tunnel đã cấu hình, đặt `CLOUDFLARE_TUNNEL_TOKEN` trong `.env` rồi chạy:

```powershell
docker compose up -d cloudflared
```

Kiểm tra `docker compose logs --tail 50 cloudflared` để xác nhận kết nối tunnel.
