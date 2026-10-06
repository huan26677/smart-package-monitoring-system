# Mang ESP32 sang mạng khác

Firmware nâng cấp dùng MQTT qua Internet và trang cấu hình tiếng Việt. Chỉ cần nạp bản nâng cấp lần đầu; những lần đổi Wi-Fi hoặc máy chủ sau đó không cần sửa code hay nạp lại. Cấp nguồn cho bộ mạch và dùng Wi-Fi 2,4 GHz hoặc điểm phát sóng điện thoại có Internet. ESP32 hiện không có modem SIM; để gửi dữ liệu khi di chuyển, cần mang theo điểm phát sóng trong phạm vi kết nối.

## Đổi mạng bằng điện thoại

1. Giữ nút **BOOT khoảng 3 giây rồi thả** khi thiết bị đang chạy. Nếu không tìm thấy mạng đã lưu, thiết bị cũng tự mở cấu hình sau khoảng 60 giây.
2. Kết nối Wi-Fi **SMART_PACKAGE_SETUP**, mật khẩu **smart1234**. Nếu điện thoại báo mạng không có Internet, chọn giữ kết nối với mạng này.
3. Mở **http://192.168.4.1** trong trình duyệt.
4. Nhập tên và mật khẩu Wi-Fi mới. Nếu chỉ sửa MQTT và giữ nguyên tên mạng, để trống ô mật khẩu Wi-Fi để giữ mật khẩu đã lưu. Với mạng mở, chọn **Mạng Wi-Fi không có mật khẩu**.
5. Giữ địa chỉ **wss://mqtt.huan2k5.id.vn/mqtt**, tài khoản **esp32-001**. Khi tài khoản không đổi, để trống ô mật khẩu MQTT để giữ mật khẩu đã lưu. Khi đổi tài khoản, nhập mật khẩu mới. Cũng có thể dán URI đầy đủ có tài khoản; thiết bị tách tài khoản/mật khẩu khi lưu và không trả mật khẩu trên trang trạng thái.
6. Chọn **Lưu và kết nối**. Thiết bị lưu vào NVS và khởi động lại để áp dụng. Rời mạng cấu hình rồi mở dashboard tại **https://monitor.huan2k5.id.vn/**. Nếu tên mạng/mật khẩu sai, chờ mạng cấu hình xuất hiện lại và sửa.

Nếu dùng chính điện thoại đang cấu hình làm điểm phát sóng, có thể nhập trước tên/mật khẩu điểm phát sóng, lưu, sau đó bật lại điểm phát sóng 2,4 GHz để ESP32 kết nối. Có thể mở trang cấu hình bằng máy tính hoặc điện thoại khác để giữ điểm phát sóng chạy liên tục.

Trang cấu hình hiển thị trạng thái Wi-Fi và MQTT riêng: chờ Wi-Fi, đang kết nối, đã kết nối, lỗi tài khoản hoặc lỗi chứng chỉ máy chủ. Mật khẩu không được điền lại vào biểu mẫu hoặc trả trong JSON. Bấm lưu với cùng tên mạng/tài khoản và các ô mật khẩu trống giữ mật khẩu hiện có. Tên mạng tối đa 32 byte; mật khẩu WPA thường từ 8 đến 63 byte hoặc 64 chữ số hex. Các lỗi nhập, trường lặp và yêu cầu thiếu mã phiên bị từ chối trước khi lưu.

## Khi mất mạng

Cảm biến, LCD, buzzer và ghi sự kiện vẫn hoạt động khi đang cấu hình hoặc mất mạng. Các sự kiện đã ghi vào NVS chờ kết nối lại; chỉ đánh dấu đồng bộ sau khi backend xác nhận lưu PostgreSQL. Bộ nhớ NVS giới hạn **128 sự kiện**, hàng đợi RAM giới hạn **32 bản ghi**; không bảo đảm lưu mọi sự kiện khi mất mạng kéo dài hoặc mất nguồn trước khi ghi flash. Toàn bộ dòng 200 mẫu dùng cho AI không được lưu ngoại tuyến.

Wi-Fi thử kết nối lại mỗi 5 giây. Sau khoảng 60 giây chưa có IP, thiết bị bật thêm điểm truy cập cấu hình mà không khởi động lại và vẫn thử mạng cũ. Khi lưu cấu hình mới, thiết bị khởi động lại ngắn để áp dụng. Nhấn BOOT không xóa lịch sử hoặc mật khẩu. Nâng cấp bằng `app-flash` giữ NVS và ID sự kiện; không dùng `erase-flash` cho thao tác đổi mạng.

## Đường truyền và máy chủ

```text
ESP32 → Wi-Fi/điểm phát sóng → Internet → mqtt.huan2k5.id.vn:443
      → Cloudflare Tunnel → EMQX WebSocket /mqtt → backend → PostgreSQL
```

Cloudflare public hostname `mqtt.huan2k5.id.vn` chuyển tiếp tới `http://emqx:8083` khi cloudflared chạy trong cùng mạng Compose. WSS dùng xác thực chứng chỉ và tài khoản MQTT riêng. Tên miền dashboard có lớp đăng nhập Cloudflare Access hiện có và đăng nhập ứng dụng; hostname MQTT phải cho phép thiết bị mở WebSocket bằng tài khoản MQTT mà không yêu cầu đăng nhập qua trình duyệt.

Máy chạy Docker phải có Internet, các dịch vụ EMQX/backend/PostgreSQL/cloudflared phải hoạt động. Tên miền không thay thế máy chủ; tắt máy chủ hoặc tunnel khiến thiết bị chưa gửi được dữ liệu. Cấu hình broker/tài khoản đang lưu trong volume EMQX, dữ liệu giám sát trong volume PostgreSQL.

## Kiểm tra sau khi đổi mạng

- Dashboard báo thiết bị đã kết nối, thời điểm nhận telemetry tiếp tục đổi.
- `pendingEvents` giảm về 0 sau khi máy chủ xác nhận các sự kiện đang chờ. `rejectedEvents` là bộ đếm lịch sử các lần không lưu được, không tự về 0 khi đổi mạng.
- Mục AI nhận đoạn cảm biến mới với nhịp hợp lệ. Chưa có mô hình được huấn luyện bằng dữ liệu đã thu thì giao diện vẫn báo chưa có mô hình.
- Nếu Wi-Fi đã kết nối nhưng MQTT chưa kết nối, kiểm tra URI WSS, `/mqtt`, tài khoản, broker và tunnel. Nếu dashboard yêu cầu Cloudflare Access, đăng nhập theo cấu hình tên miền hiện có trước khi đăng nhập ứng dụng.

## Kiểm tra đã thực hiện ngày 06/10/2026

ESP32-S3 thật tại COM3 đã kết nối WSS qua tên miền công khai, đăng ký ba topic điều khiển/xác nhận và gửi telemetry, đoạn cảm biến cùng sự kiện thật. Backend đã trả xác nhận `SAVED`; số sự kiện chờ về 0, bộ đếm từ chối lịch sử vẫn 277. Lưu lại qua trang cấu hình với hai ô mật khẩu trống giữ được kết nối sau khởi động lại. Kiểm thử C mô phỏng xác nhận chu kỳ thử lại, mở cấu hình sau 60 giây, phục hồi kết nối và thử lại khi mở portal thất bại. Chưa kiểm chứng đổi sang mạng điện thoại khác trong lần kiểm tra này.
