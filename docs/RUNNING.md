# Chạy hệ thống trên máy

Yêu cầu: Docker Desktop đang chạy và file `.env` ở thư mục gốc đã có thông tin PostgreSQL, tài khoản dashboard và tài khoản MQTT của backend. Xem các biến mẫu trong `.env.example`.

**Máy mới chưa có dữ liệu/tài khoản EMQX:** làm theo [sao lưu và chuyển máy demo](DEMO_MIGRATION.md) trước. Lệnh chạy toàn bộ bên dưới dành cho máy đã khôi phục hoặc đã tạo đủ tài khoản MQTT; backend không thể healthy nếu người dùng MQTT chưa tồn tại trong broker.

Từ thư mục gốc dự án:

```powershell
docker compose up -d --build postgres emqx backend web
docker compose ps
```

Mở dashboard tại http://localhost:8088. Backend tại http://localhost:8080.
Đăng nhập bằng `DASHBOARD_ADMIN_USERNAME` và `DASHBOARD_ADMIN_PASSWORD` trong `.env`. Mật khẩu phải có ít nhất 16 ký tự, không dùng giá trị mẫu. Bản cấu hình trên máy đã tạo mật khẩu ngẫu nhiên trong `.env`, tên tài khoản mặc định `admin`.

Mật khẩu được băm BCrypt khi khởi động backend. Phiên đăng nhập dùng cookie HttpOnly, SameSite=Lax; khi truy cập HTTPS qua Cloudflare, Nginx giữ thông tin HTTPS và Tomcat xử lý qua `server.forward-headers-strategy=native` để cookie có cờ Secure ([tài liệu Spring Boot](https://docs.spring.io/spring-boot/how-to/webserver.html)). Các API dữ liệu, Wi-Fi Anchor và xuất CSV đều cần đăng nhập. POST/DELETE và đăng nhập yêu cầu CSRF token. Phiên hết hạn sau 30 phút không có request; dashboard đang mở tiếp tục polling. Phiên cũng hết hạn khi backend khởi động lại. Endpoint health vẫn cho phép gọi để kiểm tra vận hành.

Để đổi mật khẩu, sửa `.env` rồi chạy `docker compose up -d backend`. Không lưu mật khẩu trong mã frontend hoặc commit `.env`.
Web và API được mở trên localhost và qua Cloudflare Tunnel. ESP32 có thể dùng `wss://mqtt.huan2k5.id.vn/mqtt` để kết nối qua Internet từ Wi-Fi 2,4 GHz hoặc điểm phát sóng điện thoại. Đổi Wi-Fi/MQTT trên trang cấu hình thiết bị, không cần sửa mã và nạp lại; xem [hướng dẫn mang thiết bị sang mạng khác](MOBILE_NETWORK.md). MQTT TCP cổng 1883 vẫn dùng được trong LAN nếu tài khoản và quyền topic phù hợp.

Compose đợi EMQX sẵn sàng rồi mới chạy backend; đợi backend kết nối được PostgreSQL và đăng ký các topic MQTT rồi mới chạy web. Lần khởi động đầu có thể mất vài phút để build và khởi tạo dịch vụ.

## Tài khoản MQTT của backend

`MQTT_USERNAME` và `MQTT_PASSWORD` trong `.env` là tài khoản MQTT riêng của backend, khác tài khoản đăng nhập dashboard và tài khoản `esp32-001` của thiết bị. Tài khoản phải tồn tại trong cơ sở dữ liệu xác thực EMQX. Backend gửi thông tin này khi kết nối lần đầu và khi kết nối lại; mật khẩu không được ghi vào log hoặc mã nguồn. Docker Compose yêu cầu hai biến này để tránh khởi động backend bằng kết nối ẩn danh khi broker đã bật xác thực.

Trên máy hiện tại đã tạo tài khoản MQTT `smart-package-backend`, không có quyền superuser, và lưu mật khẩu ngẫu nhiên trong `.env`. Khi cài mới: khởi động `emqx` và `postgres`, mở dashboard EMQX tại `http://localhost:18083`, tạo bộ xác thực Password-Based/Built-in Database nếu chưa có, rồi tạo người dùng backend với tài khoản/mật khẩu khớp `.env`. Cấp quyền đăng ký `smart-package/+/telemetry`, `smart-package/+/event`, `smart-package/+/location-scan`, `smart-package/+/motion-window`; quyền gửi `smart-package/+/event-ack`, `smart-package/+/motion-ack`, `smart-package/+/motion-control`. Tạo thêm người dùng MQTT `esp32-001` khớp thông tin đã lưu trên ESP32, với quyền gửi telemetry/event/location-scan/motion-window của chính thiết bị và nhận event-ack/motion-ack/motion-control. Tiếp đó khởi động backend và web. Chi tiết cài mới và khôi phục có trong [DEMO_MIGRATION.md](DEMO_MIGRATION.md).

EMQX được cố định ở phiên bản 6.3.1, nạp mặc định từ `emqx-docker/base.hocon`. Cổng TCP 1883 và WebSocket 8083 đều bật xác thực. Các thay đổi và người dùng đang lưu trong volume EMQX vẫn được giữ. Không dùng tệp YAML làm cấu hình chạy cho phiên bản này; [tài liệu EMQX](https://docs.emqx.com/en/emqx/latest/guides/configuration/configuration.html) mô tả các tệp HOCON và thứ tự ghi đè cấu hình.

Nếu log backend báo `Not authorized to connect`, kiểm tra tài khoản MQTT trong `.env` có khớp người dùng EMQX hay không. Sau khi đổi thông tin backend, chạy `docker compose up -d backend`. Nếu `/api/health/mqtt` trả 503 trong khi database vẫn OK, ESP32 có thể vẫn kết nối broker nhưng backend chưa nhận được dữ liệu. Dashboard hiển thị **Máy chủ mất kết nối MQTT** và đánh dấu các chỉ số từ lần nhận cuối là **Dữ liệu cũ**; khi máy chủ nhận dữ liệu lại, giao diện tự trở về trạng thái hiện tại.

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

## Sự kiện và mất kết nối

Thiết bị hiện tại có ID `esp32-001`, dành cho một ESP32. Không xóa toàn bộ NVS hoặc đặt lại số ID sự kiện nếu muốn tiếp tục cùng lịch sử trên server.

Va đập bắt đầu từ 2,5 g; firmware giữ đỉnh và mức cao nhất trong đợt, kết thúc sau 80 ms liên tục dưới 1,8 g hoặc sau tối đa 1 giây. LCD, buzzer và bản ghi dùng kết quả của đợt. Thời lượng là khoảng từ mẫu kích hoạt đầu đến mẫu cuối trên ngưỡng 1,8 g, cộng một chu kỳ danh định 10 ms, không phải phép đo chính xác độ rộng xung analog. Cờ `saturated` bật nếu có trục đạt ít nhất 15,9 g trong thang ±16 g.

ESP32 có lịch sử NVS tối đa 128 sự kiện. Chỉ bản ghi đã được backend xác nhận mới được thay thế khi đầy. Nếu tất cả đều chưa đồng bộ, bản ghi mới bị từ chối và tăng bộ đếm `rejectedEvents`; dashboard hiển thị cảnh báo khi nhận telemetry. Trước khi ghi flash, có hàng đợi RAM 32 bản ghi để giảm việc chặn vòng đọc cảm biến; mất nguồn trước khi ghi xong có thể mất các bản ghi còn trong RAM. Dung lượng này hữu hạn, không bảo đảm lưu mọi sự kiện khi mất kết nối kéo dài.

ESP32 nhận xác nhận JSON `{"deviceId":"esp32-001","eventId":123,"status":"SAVED"}` trên topic `smart-package/esp32-001/event-ack`, QoS 1, không retained. Backend chỉ gửi sau khi giao dịch cơ sở dữ liệu hoàn tất, hoặc khi bản ghi đó đã tồn tại. Thiết bị gửi lại sau 10 giây nếu chưa nhận xác nhận. PUBACK của EMQX không đánh dấu đã đồng bộ. Khi mất Wi-Fi, thiết bị tiếp tục theo dõi và thử lại mỗi 5 giây. Sau khoảng 60 giây chưa nhận được IP, thiết bị mở thêm mạng `SMART_PACKAGE_SETUP` để đổi cấu hình; chế độ APSTA giữ việc thử kết nối Wi-Fi và lấy mẫu cảm biến.

Lịch sử cũ V1/V2 được chuyển sang V3 khi khởi động mà giữ ID, giờ có sẵn và cờ đồng bộ. Dữ liệu cũ không có thời lượng/kiểm tra giới hạn đo được gửi các trường mới là `null`. Những sự kiện firmware cũ đã đánh dấu đồng bộ mà server chưa lưu không thể tự phục hồi bằng lần nâng cấp này.

Bảng lịch sử tách giờ xảy ra (`timestamp` trên thiết bị) và giờ nhận (`receivedAt` trên backend). Sự kiện chưa đồng bộ NTP hiển thị rõ trạng thái thiếu giờ và bị loại khỏi bộ lọc thời gian. CSV xuất toàn bộ kết quả lọc, tối đa 10.000 sự kiện, gồm cả hai thời điểm. Cảnh báo va đập/rơi được giữ đến khi nhấn “Đã xem cảnh báo”; xác nhận được lưu trong trình duyệt, áp dụng cho các sự kiện đang hiển thị trong nhóm gần nhất, không phải xác nhận lưu vào cơ sở dữ liệu.

## Nạp firmware

Giữ nguyên bảng phân vùng và NVS khi nâng cấp. Trong ESP-IDF PowerShell, từ `firmware-esp32`:

```powershell
idf.py build
idf.py -p COM3 app-flash
idf.py -p COM3 monitor
```

Firmware dùng `CONFIG_MQTT_POLL_READ_TIMEOUT_MS=50` trong `sdkconfig` và `sdkconfig.defaults`. Với ESP-MQTT của ESP-IDF 5.5.5, khoảng chờ mặc định 1.000 ms làm hàng đợi tăng khi đồng thời gửi telemetry mỗi giây và đoạn cảm biến mỗi hai giây; dữ liệu có thể đến muộn và lần thu bị thiếu đoạn. Giữ cấu hình 50 ms khi build hoặc tạo lại `sdkconfig`.

Đổi COM3 theo cổng USB thực tế. `app-flash` chỉ nạp phân vùng ứng dụng. Đặt kiện hàng cố định khi firmware lấy mẫu hiệu chuẩn ban đầu, rồi kiểm tra dashboard có telemetry mới và bộ đếm chờ đồng bộ giảm về 0 khi server hoạt động.

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

Hostname MQTT trỏ tới `http://emqx:8083` nếu cloudflared chạy trong cùng mạng Docker. ESP32 dùng URI WSS với đường dẫn `/mqtt`, tài khoản/mật khẩu MQTT nhập riêng trên trang cấu hình. WebSocket công khai đang yêu cầu xác thực; EMQX lưu cấu hình và tài khoản trong volume `smart_package_emqx_data`. Giữ máy chủ, Docker và tunnel chạy để thiết bị ngoài LAN có thể gửi dữ liệu. [ESP-MQTT](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/protocols/mqtt.html) hỗ trợ WSS; [Cloudflare](https://developers.cloudflare.com/network/websockets/) chuyển tiếp WebSocket.


## Dòng đo dùng cho AI

Firmware dùng FIFO của IMU, nhịp danh định 100 Hz, 200 mẫu sáu trục cho một đoạn. Dữ liệu trục là mẫu thật trong FIFO; dấu thời gian là ước tính theo thứ tự gói và đồng hồ ESP32. Mốc được neo lùi 20 ms, có chỉnh nhẹ chênh lệch đồng hồ giữa các lô; không phải đồng hồ thời gian riêng của từng mẫu trên IMU. FIFO có giới hạn; tràn/lỗi I2C khiến đoạn đang xây dựng bị bỏ và có thể làm thiếu đoạn thu. Tiêu chí chất lượng backend vẫn giữ nguyên.

[Tài liệu MPU-6500 của TDK](https://invensense.tdk.com/wp-content/uploads/2020/06/PS-MPU-6500A-01-v1.3.pdf) mô tả FIFO 512 byte. Với gói 14 byte, bộ đệm giữ tối đa 36 mẫu hoàn chỉnh, tương đương khoảng 0,36 giây ở nhịp danh định. Không thể cam kết giữ đủ mẫu nếu thiết bị bị chặn lâu hơn dung lượng này.

Nhãn “Đã kết nối” trên dashboard có nghĩa thiết bị vừa gửi dữ liệu; giá trị được cập nhật định kỳ. Thông báo “Chưa nhận đoạn cảm biến mới” ở mục AI là tình trạng riêng của dòng mẫu AI.
