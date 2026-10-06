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
.\mvnw.cmd clean test
```

Kết quả mong đợi:

- Failures: 0
- Errors: 0
- BUILD SUCCESS

### MQTT recovery tests

`MqttSubscriberTests` kiểm tra kết nối thất bại lần đầu, giữ tài khoản/mật khẩu khi kết nối lại và đăng ký lại topic sau mất kết nối, thử lại khi đăng ký topic thất bại, và từ chối cấu hình có mật khẩu nhưng thiếu tài khoản. Các test dùng MQTT client giả lập, không cần broker thật; kết nối không xác thực vẫn được kiểm tra khi chạy ngoài Compose.

`MqttMessageServiceTests` kiểm tra xác nhận sau lưu thành công, xác nhận bản ghi trùng, không xác nhận khi lưu lỗi, từ chối topic/đo không hợp lệ và tương thích JSON của firmware cũ. Tổng kiểm thử backend hiện tại: 29 unit test (gồm 12 kiểm thử AI).

## Sửa lỗi xác thực MQTT của backend ngày 06/10/2026

Nguyên nhân xác nhận: EMQX 6.3.1 yêu cầu xác thực tại TCP 1883 nhưng backend chưa gửi tài khoản. Log liên tục báo `Not authorized to connect`, MQTT health trả 503, PostgreSQL health vẫn OK. Telemetry dừng lưu lúc 15:18:32; ESP32 vẫn kết nối WSS và broker nhận dữ liệu nhưng không có backend đăng ký topic. Dockerfile cũ chép `emqx.yaml`, không nạp cấu hình HOCON cho bản EMQX đang chạy.

- Backend nhận tài khoản/mật khẩu qua `MQTT_USERNAME`/`MQTT_PASSWORD`, gửi lại khi kết nối lại. Tạo tài khoản `smart-package-backend`, không có quyền superuser; kiểm tra CONNACK thành công và SUBACK cho cả bốn topic. Mật khẩu ngẫu nhiên chỉ lưu trong `.env` được Git bỏ qua.
- Dùng `base.hocon`, giữ xác thực trên TCP/WebSocket, cố định EMQX 6.3.1 và giữ volume dữ liệu. Xuất bản sao lưu EMQX trước thay đổi, sao lưu `.env`; các bản sao nằm trong `firmware-esp32/build/backups/mqtt-auth-20261006/`, được Git bỏ qua.
- 29 unit test backend đạt; ESLint và production build frontend đạt. Build và chạy lại EMQX/backend/web thành công; database và MQTT health OK. Không nạp lại hoặc xóa NVS của ESP32.
- Chrome với thiết bị thật: telemetry tăng từ 40611 lên 40614, `pendingEvents=0`, giữ bộ đếm lịch sử 277; phần AI có đoạn mới với nhịp hợp lệ. Các sự kiện đang chờ, gồm ID 1115–1118, được lưu vào PostgreSQL khi backend kết nối lại.
- Khởi động lại broker: backend mất kết nối lúc 15:58:32 và tự kết nối, đăng ký lại bốn topic lúc 15:58:54; ESP32 cũng tự kết nối WSS, giữ ba topic. Sau đó Chrome xác nhận telemetry tăng từ 40759 lên 40762, số sự kiện chờ vẫn 0 và đoạn AI mới có nhịp hợp lệ.
- Phản hồi chỉ được thay thế trong trình duyệt để kiểm tra giao diện: MQTT health 503 hiển thị lỗi máy chủ, MQTT khỏe nhưng ESP32 không có dữ liệu hiển thị mất kết nối thiết bị, lỗi cập nhật API đánh dấu dữ liệu đang giữ là cũ. Hiển thị thời điểm nhận cuối, dùng nhãn trung tính, phục hồi giao diện khi có dữ liệu mới. Màn hình 390 px không tràn ngang và không có lỗi JavaScript. Các phản hồi giả lập này không ghi dữ liệu lên server hay thay đổi nhãn huấn luyện.

## Firmware trên máy tính

Các test C dùng mẫu cảm biến và NVS giả lập, không cần phần cứng. Trên Linux có trình biên dịch C, từ `firmware-esp32`: `sh tests/host/run.sh`. Trên Windows có Docker, từ thư mục gốc:

```powershell
$firmwarePath = (Resolve-Path firmware-esp32).Path
docker run --rm --mount "type=bind,source=$firmwarePath,target=/src,readonly" -w /src alpine:3.21 sh -c 'apk add --no-cache gcc musl-dev >/dev/null && sh tests/host/run.sh'
```

Kiểm tra gom đỉnh, mức mạnh nhất, một đợt một va đập, free fall → drop, chạm giới hạn đo, thời gian uptime quay vòng, hàng đợi NVS đầy không ghi đè bản chưa đồng bộ, lỗi ghi cờ đồng bộ, phục hồi sau khởi động lại và chuyển lịch sử V2.

`network_config_test.c` kiểm tra URI MQTT/WSS, tách tài khoản, giải mã form, chuỗi quá dài, trường lặp và giới hạn mật khẩu Wi-Fi. `wifi_recovery_test.c` chạy mã Wi-Fi manager thật với API ESP-IDF mô phỏng: thử lại mỗi 5 giây, mở AP cấu hình sau 60 giây, thử lại khi mở portal lỗi, kết nối phục hồi trước/sau thời hạn, chờ DHCP và sao chép đủ SSID/mật khẩu ở giới hạn. Cả năm chương trình được chạy bởi `tests/host/run.sh`.

## Kiểm tra kết nối WSS và trang cấu hình ngày 06/10/2026

- Build ESP-IDF 5.5.5 thành công, chạy năm chương trình kiểm thử C thành công. Sao lưu flash 0x0–0x310000 trước nâng cấp, chỉ nạp phân vùng ứng dụng trên ESP32-S3 COM3.
- WSS công khai: WebSocket trả 101, thông tin MQTT đúng được CONNACK thành công và đăng ký topic; mật khẩu sai bị từ chối. Không gửi sự kiện giả hoặc dữ liệu huấn luyện qua phép thử này.
- Thiết bị thật kết nối `wss://mqtt.huan2k5.id.vn/mqtt`, broker nhận client `esp32-001` qua địa chỉ cloudflared, đăng ký ba topic. Telemetry và đoạn AI mới tới backend, sự kiện thật được xác nhận lưu PostgreSQL, `pendingEvents=0`, giữ bộ đếm từ chối lịch sử 277.
- Portal thật ở chế độ APSTA: trang và thông báo tiếng Việt, status không lộ mật khẩu; sai token trả 403. Mật khẩu Wi-Fi ngắn, đổi SSID không có mật khẩu, broker sai, đổi tài khoản MQTT thiếu mật khẩu, trường SSID lặp, body quá dài và NUL mã hóa đều trả 400. Status giữ nguyên sau các yêu cầu bị từ chối. Lưu cùng mạng/tài khoản với hai mật khẩu trống thành công; ESP32 khởi động lại và kết nối WSS bằng mật khẩu đã lưu.
- Dashboard qua localhost: database/MQTT health OK, thiết bị ONLINE, telemetry mới và đoạn cảm biến AI có nhịp hợp lệ. Tên miền dashboard đang có Cloudflare Access nên yêu cầu đăng nhập lớp đó khi kiểm tra qua trình duyệt.
- Chrome trên portal thật: hai ô mật khẩu trống, hiển thị đã kết nối, polling giữ bản nhập chưa lưu, phản hồi lỗi tiếng Việt và cho thử lại, màn hình 390 px không tràn ngang, không lỗi JavaScript. Chrome dashboard sau bản nạp cuối: đăng nhập thành công, telemetry tăng từ ID 38899 lên 38902 qua polling; đoạn AI mới có nhịp hợp lệ và chưa có mô hình, không lỗi JavaScript.

Chưa thực hiện kiểm tra trên một Wi-Fi/điểm phát sóng khác trong lượt này. Hướng dẫn thực hiện và giới hạn khi mất mạng nằm trong [MOBILE_NETWORK.md](MOBILE_NETWORK.md).

## Đăng nhập và dữ liệu thực

1. Truy cập dashboard chưa đăng nhập: phải thấy form đăng nhập; GET `/api/devices`, Wi-Fi Anchor và CSV phải trả 401.
2. Đăng nhập sai mật khẩu phải thất bại. Sau đăng nhập đúng, đọc dữ liệu được; POST/DELETE không có CSRF token phải bị chặn. Đăng xuất phải vô hiệu hóa phiên cũ.
3. Gửi sự kiện vào MQTT, đăng ký topic `event-ack`: receipt `SAVED` chỉ đến sau khi lưu PostgreSQL; gửi lại cùng ID có receipt nhưng không tăng số dòng.
4. Dừng backend trong khi EMQX vẫn chạy: thiết bị chưa nhận receipt phải giữ sự kiện. Bật backend và gửi lại phải lưu đúng một lần. Có PUBACK của broker chưa chứng minh lưu database.
5. So sánh giờ xảy ra và giờ nhận khi đồng bộ sự kiện cũ. Kiểm tra bộ lọc loại, mức, thời gian; phân trang; thống kê tổng; CSV toàn bộ kết quả lọc. Timestamp 0 phải được hiển thị thiếu NTP và loại khỏi bộ lọc thời gian.
6. Sau một va đập, telemetry về NORMAL vẫn giữ cảnh báo sự kiện đến khi nhấn “Đã xem cảnh báo”; tải lại trang giữ xác nhận trong cùng trình duyệt.

Kịch bản kiểm thử vật lý và biểu mẫu ghi số liệu nằm trong [EXPERIMENTS.md](EXPERIMENTS.md).

## Kết quả kiểm tra trên máy ngày 06/10/2026

- Backend: 16 unit test đạt; frontend: ESLint và production build đạt; firmware: ESP-IDF build và hai chương trình kiểm thử C đạt.
- EMQX/PostgreSQL thật: xác nhận sau commit, gửi trùng không tạo thêm dòng, lỗi INSERT có PUBACK nhưng không có receipt, gửi lại sau phục hồi lưu được. Đã dừng và bật backend để kiểm tra gửi lại sau mất kết nối; dữ liệu và trigger thử nghiệm dùng ID riêng đã được xóa.
- API: chặn dữ liệu khi chưa đăng nhập, từ chối mật khẩu sai, kiểm tra CSRF, đăng xuất vô hiệu hóa phiên. HTTPS được mô phỏng bằng header từ proxy để xác nhận cookie Secure/HttpOnly/SameSite; chưa kiểm tra trực tiếp tên miền công khai trong lần chạy này.
- Chrome: đăng nhập, telemetry tự cập nhật, lọc thống kê, tải CSV, thêm/xóa Wi-Fi Anchor, cảnh báo tồn tại qua polling và xác nhận tồn tại qua tải lại trang, đăng xuất. Màn hình 390 px không tràn ngang toàn trang, không có lỗi JavaScript. Cảnh báo được kiểm tra bằng dữ liệu chỉ thay thế trong trình duyệt, không ghi sự kiện giả lên thiết bị thật.
- ESP32-S3 thật tại COM3: đã sao lưu vùng flash chứa firmware và NVS, nạp riêng ứng dụng, chuyển 50 bản ghi V2 sang V3. Sau lần nạp cuối, thiết bị ONLINE, telemetry NORMAL tiếp tục cập nhật, `pendingEvents=0`, `rejectedEvents=0`; database và MQTT health đều OK.

Chưa thực hiện thử thả/va đập vật lý hoặc đo độ chính xác cảm biến. Kết quả trên xác nhận vận hành và logic phần mềm, không thay thế thực nghiệm theo kế hoạch. Backup PostgreSQL và flash trước nâng cấp nằm trong `firmware-esp32/build/backups/` được Git bỏ qua.

Kiểm tra vận hành với broker thật:

1. Chạy các dịch vụ theo [RUNNING.md](RUNNING.md).
2. Xác nhận `/api/health/mqtt` trả HTTP 200 và thiết bị ESP32 cập nhật telemetry.
3. Dừng EMQX bằng `docker compose stop emqx`: MQTT health phải trả HTTP 503, API database vẫn trả HTTP 200.
4. Khởi động EMQX bằng `docker compose start emqx`: backend phải tự kết nối lại, đăng ký đủ 4 topic và nhận dữ liệu mới.
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


## Kiểm thử AI

- Backend: `MotionFeaturesTests` kiểm tra một g khi đứng yên, đỉnh va đập ngắn, tốc độ góc, bão hòa và thời gian mẫu lỗi. `ForestModelTests` kiểm tra nhánh float32, kết quả chưa chắc chắn, cấu trúc cây sai và mô hình Python xuất sang Java. Tệp trong `src/test/resources/ai/` là dữ liệu giả lập ghi rõ `synthetic-test`, không được nạp vào hệ thống thực.
- `MotionServiceTests` kiểm tra đoạn trực tiếp không ghi mẫu thô/không tạo kết quả AI giả, không có receipt khi lưu lỗi, xử lý bản trùng và lần thu đã hoàn thành, hạn chế nhãn và quyền sở hữu dữ liệu.
- Công cụ huấn luyện: từ thư mục gốc chạy `ai-training/.venv.local/Scripts/python.exe -m unittest discover -s ai-training -p "test_*.py" -v`. Bốn kiểm thử xác nhận tách nhóm, khớp xác suất xuất cây, từ chối dữ liệu giả khi dùng thực và từ chối thiếu nhóm/bão hòa/NaN.
- Kiểm tra tích hợp EMQX/PostgreSQL: thiết bị thử riêng, đăng nhập/CSRF, gắn nhãn, xem 200 mẫu, bản trùng, loại mẫu chất lượng kém khỏi JSON xuất. Khi INSERT thất bại, broker vẫn PUBACK nhưng không có receipt `SAVED`; gửi lại sau phục hồi lưu được. Mô hình thử bị từ chối. Dữ liệu và trigger thử được dọn sau kiểm tra.

### Giao thức AI

- `smart-package/esp32-001/motion-window`: chuỗi mẫu, phiên bản đặc trưng, mã khởi động, số đoạn, mã lần thu và nhãn quy tắc; không tự tạo nhãn quan sát.
- `smart-package/esp32-001/motion-control`: lệnh START/STOP từ dashboard, không retained.
- `smart-package/esp32-001/motion-ack`: xác nhận theo mã lần thu và số đoạn, chỉ gửi sau transaction lưu thành công.
- API có đăng nhập/CSRF: `/api/devices/{deviceId}/ai`, `captures`, `captures/{id}/windows`, `windows/{id}`, `dataset.json`, `model`.

Mô hình trên thiết bị thật vẫn chưa huấn luyện; các kiểm thử phần mềm không phải đánh giá khả năng phân biệt va đập vật lý. Xem [AI_GUIDE.md](AI_GUIDE.md) để thu các buổi thử có nhãn quan sát.

### Kết quả vận hành AI trên ESP32-S3 thật ngày 06/10/2026

- Firmware đã nạp riêng ứng dụng tại COM3, bảo toàn NVS và cấu hình mạng. Nhịp lấy mẫu được ưu tiên; tác vụ đóng gói chạy trên lõi còn lại, gửi telemetry qua hàng đợi. Khi kết nối lại, dữ liệu trực tiếp chỉ giữ đoạn mới nhất; STOP hủy phần thu còn chờ.
- Thu 10 giây qua Chrome: nhận đủ 5/5 đoạn, mỗi đoạn đúng 200 mẫu với thời gian tăng; cả năm đoạn của lần thu cuối đủ điều kiện chất lượng. Đồ thị sáu trục, phân trang và tải JSON hoạt động; các đoạn chưa gắn nhãn không được xuất vào tập huấn luyện.
- Kiểm tra độc lập 10 đoạn trực tiếp tiếp theo: 10/10 có nhịp hợp lệ, khoảng cách mẫu quan sát 5–16 ms. Đây là kiểm tra vận hành khi cảm biến đặt yên, không phải độ chính xác phân loại vật lý.
- Dashboard tiếng Việt trên màn hình máy tính/390 px: không tràn ngang toàn trang và không có lỗi JavaScript. Đăng nhập, CSV, quản lý mốc Wi-Fi và cảnh báo giữ qua tải lại tiếp tục hoạt động.
- Các lần thu kỹ thuật, nhãn thử, trigger kiểm tra và dấu vết MQTT tạm đã được dọn. Thiết bị thật chưa có mô hình/nhãn quan sát; cần người dùng thu các buổi thử độc lập.
- Backup trước AI: `firmware-esp32/build/backups/database-before-ai.sql` và `esp32-before-ai.bin`, đều được Git bỏ qua.

### Kiểm tra sau khi sửa độ trễ MQTT ngày 06/10/2026

- Khi chạy lâu với khoảng chờ MQTT mặc định 1.000 ms, hàng đợi truyền tăng dần; lần thu đứng yên 30 giây chỉ nhận kịp 3/15 đoạn. Người thử xác nhận giữ yên, ba đoạn đã nhận được gắn nhãn Bình thường; dữ liệu được giữ lại.
- Đặt khoảng chờ MQTT thành 50 ms trong `sdkconfig` và `sdkconfig.defaults`, build thành công và nạp riêng ứng dụng lên ESP32-S3 tại COM3. Backup trước sửa nằm trong `firmware-esp32/build/backups/esp32-before-mqtt-poll-20261006-0313.bin`.
- Sau khi khởi động lại Docker Desktop, cả PostgreSQL, EMQX, backend và web chạy; ESP32 tự kết nối lại. Sáu đoạn trực tiếp liên tiếp đến cách nhau khoảng 1,88–2,12 giây, độ trễ không tăng dần; 5/6 đoạn có nhịp đo hợp lệ.
- Lần thu đứng yên tiếp theo nhận đủ 15/15 đoạn trong khoảng 30 giây; 14 đoạn đủ điều kiện, một đoạn bị loại vì nhịp đo không đều. Chỉ gắn nhãn sau khi người thực hiện xác nhận thao tác. Đây chưa phải đánh giá phân loại AI.
- `pendingEvents=0`; `rejectedEvents=277` là bộ đếm đã có trước lượt thử và không tăng trong lượt đứng yên. Không xóa bộ đếm hoặc dữ liệu cũ để làm sạch kết quả.


## Kiểm tra kết nối và lấy mẫu FIFO ngày 06/10/2026

- Dashboard đã có dữ liệu ESP32; nhãn “Đang kết nối” khi `online=true` gây hiểu nhầm. Đã đổi thành “Đã kết nối” và cập nhật container web. Trình duyệt kiểm tra đăng nhập, hai lần polling có ID telemetry tăng, không lỗi JavaScript hay lỗi API.
- Cloudflare Tunnel đăng ký kết nối; tên miền chuyển khách chưa xác thực sang Cloudflare Access. Chưa kiểm tra dashboard công khai với phiên Access của người dùng; dashboard tại localhost hoạt động.
- Lượt Va đập trước sửa nhận 8/15 đoạn, không đoạn nào đạt nhịp đo. Giữ nguyên dữ liệu, không đưa vào huấn luyện. Kiện thử thực tế là bộ mạch breadboard khoảng 100 g, chưa có hộp kiện; không suy rộng kết quả sang kiện hàng thực.
- Firmware đọc FIFO thay cho polling thanh ghi; đặt nhịp danh định 100 Hz. Tách LCD, in console và telemetry khỏi luồng xử lý mẫu. Tràn FIFO/lỗi đọc/bất đồng nhịp lớn sẽ bỏ đoạn AI đang xây dựng, không nội suy giá trị cảm biến. Mẫu FIFO không có dấu thời gian phần cứng: thời gian được ước tính từ thứ tự gói, đồng hồ ESP32 và nhịp danh định, neo lùi 20 ms rồi chỉnh nhẹ sai lệch đồng hồ. Không dùng các thời gian này để khẳng định độ chính xác thời gian va đập ở cấp mili giây.
- Kiểm thử C bộ phát hiện, lưu sự kiện và FIFO đã qua. Test FIFO bao gồm chuyển đổi đơn vị, thứ tự thời gian, gói bổ sung giữa hai lần đọc, tràn trước/sau đọc, sai nhịp, đọc count chậm và lỗi I2C. Build ESP-IDF 5.5.5 thành công; binary 0xf73f0 byte. Đã nạp riêng app qua COM3, xác minh hash.
- Kiểm tra đứng yên sau nạp: 6/6 đoạn trực tiếp có nhịp hợp lệ, không bão hòa; cửa sổ khoảng 1.980 ms, chu kỳ trung bình 9,90 ms. Quan sát serial 10 giây không báo lỗi FIFO. Đây là kiểm tra truyền/lấy mẫu, chưa gắn nhãn thử nghiệm và chưa chứng minh nhịp khi va đập.
- Chưa huấn luyện mô hình. Dữ liệu hợp lệ đã có nhãn vẫn là Bình thường 17 đoạn/2 nhóm, Rung lắc 13 đoạn/1 nhóm, Va đập 0. Cần thu lại cả ba lớp bằng cùng cách lấy mẫu FIFO trước huấn luyện để tránh mô hình học khác biệt cách thu.
- Sao lưu trước FIFO gồm vùng từ 0x9000, dài 0x108000 byte, tại `firmware-esp32/build/backups/esp32-before-fifo-20261006.bin`. Không tự khôi phục toàn bộ tệp này vì chứa NVS cũ.

- Kiểm tra Va đập breadboard 10 giây sau sửa: 4/5 đoạn được lưu trước khi dừng; cả 4 hợp lệ về nhịp và không bão hòa, đỉnh 3,08–6,53 g. Chưa gắn nhãn vì đang chờ xác nhận thao tác từ người dùng. Không dùng ngưỡng để suy ra nhãn quan sát.

- Lượt Va đập breadboard 30 giây sau sửa nhận đủ 15/15 đoạn: 14 đoạn hợp lệ; đoạn 13 bão hòa bị loại. Lượt ngắn 10 giây có gián đoạn theo xác nhận của người dùng, giữ toàn bộ chưa gắn nhãn. Chưa dùng hai lượt này để báo độ chính xác mô hình.

- Sau xác nhận thao tác, lượt Va đập 30 giây được gắn nhãn (14 đoạn hợp lệ), lượt Bình thường 30 giây được gắn nhãn (15/15 hợp lệ). Lượt Rung lắc tiếp theo nhận đủ 15/15 hợp lệ, chờ xác nhận người dùng. Bộ dữ liệu dành cho huấn luyện do công cụ thu xuất chỉ chọn cùng chế độ FIFO V3; dữ liệu polling cũ và các lượt chưa có nhãn vẫn giữ trong cơ sở dữ liệu.

## Sửa cảnh báo lịch sử bộ nhớ sự kiện - 06/10/2026

- Telemetry thực trong 60 mẫu gần nhất có `pendingEvents=0`, `rejectedEvents=277` không tăng. Bộ đếm không lưu được sự kiện là số tích lũy; không chứng minh bộ nhớ hiện tại đang đầy.
- Dashboard chuyển thông báo lịch sử sang màu vàng, ghi rõ số tích lũy, thêm xác nhận theo từng thiết bị trong trình duyệt. Sau xác nhận, số liệu vẫn xem được trong mục thu gọn. Khi bộ đếm thay đổi, thông báo mở lại; khi tăng so với lần xác nhận hoặc giá trị đầu trang, hiển thị cảnh báo đỏ.
- Nút Làm mới cập nhật dữ liệu mà không tháo giao diện con, giữ trạng thái thông báo. Sự kiện va đập/rơi chưa xác nhận ghi rõ là lịch sử và vẫn yêu cầu người dùng xem cảnh báo.
- Docker build frontend đạt. Kiểm tra Chrome đăng nhập thật đạt: cảnh báo lịch sử màu vàng, đồng bộ 0, xác nhận qua tải lại trang, lỗi mới sau xác nhận hiện màu đỏ, số sự kiện chờ vẫn hiện, mất kết nối không báo đã đồng bộ, bộ đếm thấp hơn xác nhận cũ vẫn hiện thông báo, không lỗi JavaScript. Các trường hợp lỗi giả lập chỉ chặn phản hồi GET trong trình duyệt kiểm tra; không ghi dữ liệu giả vào MQTT/database.
- Không xóa lịch sử sự kiện, dữ liệu thu AI hoặc đặt lại bộ đếm 277.

## Các mục dashboard thu gọn - 06/10/2026

- Tám mục có thanh tiêu đề mở/đóng: thông báo, chỉ số hiện tại, lịch sử cảm biến, lịch sử sự kiện, AI, bản đồ, lịch sử vị trí và mốc Wi-Fi. Chỉ số hiện tại mở mặc định; trạng thái thiết bị hiện ở đầu trang. Thanh tiêu đề có thông tin tóm tắt và số lượng khi phù hợp.
- Mỗi mục nhớ trạng thái mở/đóng trong trình duyệt, dùng nút có `aria-expanded` và `aria-controls`, thao tác được bằng Enter/Space. Sau lần mở đầu, nội dung giữ nguyên khi thu gọn để bảo toàn biểu mẫu, bộ lọc và phiên thu AI; thông báo vẫn được cập nhật trong mục thu gọn.
- Bản đồ điều chỉnh kích thước bằng ResizeObserver sau khi mở lại. Biểu đồ và bảng không làm tràn chiều ngang ở màn hình 375 px. Chỉ số trên màn hình lớn xếp thành một hàng năm ô.
- Docker build đạt; ESLint của component CollapsibleSection đạt. Chrome đăng nhập thật kiểm tra tám mục, thao tác bàn phím, nhớ bố cục qua tải lại, giữ trường AI qua thu gọn/Làm mới, giữ bộ lọc sự kiện và biểu mẫu Wi-Fi, mở lại biểu đồ/bản đồ, tám mục trên điện thoại và không lỗi JavaScript. Không gửi yêu cầu ghi dữ liệu backend trong kiểm tra bố cục.
- Ở viewport 1365 px, chiều cao mặc định khoảng 1275 px, so với 8690 px khi mở toàn bộ mục (cùng dữ liệu thử).

## Script đóng gói và kiểm tra demo - 06/10/2026

- Hai script dùng thư viện chuẩn Python: `Lenh/don_dep_dong_goi.py` và `Lenh/chay_demo.py`. Script dọn chỉ xóa danh sách thư mục có thể tạo lại, tạo bản sao lưu trước khi xóa và chép riêng `build/backups`; không xóa Docker volume. Chế độ xem trước trên dự án thật ước tính gần 596 MB, chưa xóa các thư mục nặng của dự án.
- Kiểm thử riêng xác nhận giữ nguồn, `.env`, dữ liệu AI, bản sao lưu; lỗi sao lưu ngăn việc dọn; từ chối thoát khỏi thư mục dự án hoặc đi theo junction; xóa cache có junction bên trong không xóa dữ liệu ở đích. Các mật khẩu có `$`, dấu nháy, backslash, khoảng trắng và Unicode được kiểm tra qua Compose thật để giữ nguyên khi chuyển máy.
- Đã chạy chức năng sao lưu thật và tự khôi phục lên PostgreSQL/EMQX riêng với dữ liệu trống. Có lịch sử telemetry/sự kiện, các lần thu/đoạn AI; tài khoản backend kết nối thành công sau nhập. Gọi lại bước chuẩn bị giữ dữ liệu đã có. Container kiểm tra được xóa sau khi hoàn tất; không bàn giao tunnel sang container thử.
- Script demo được chạy ở cả chế độ kiểm tra và khởi động/build trên hệ thống hiện tại. Local đạt: database/MQTT OK, API dữ liệu yêu cầu đăng nhập, tài khoản dashboard đúng, telemetry ESP32 thật có ID tăng, pendingEvents=0 và đoạn AI mới hợp lệ. WSS public nhận telemetry thật. Kiểm tra không tạo lượt thu hoặc gắn nhãn AI.
- Hostname web chuyển yêu cầu script sang Cloudflare Access. Script báo còn bước đăng nhập/kiểm tra trình duyệt, trả mã 2 và không coi web public đã được xác minh. `--khong-public` đạt mã 0 cho demo local. LCD/còi vẫn cần quan sát bằng thao tác thật; mô hình AI hiện chưa có được ghi rõ trong kết quả.
- Báo cáo trong `demo-private/lenh/` không chứa mật khẩu. Chưa kiểm tra thực tế trên máy tính khác hoặc thử nhánh mở Docker Desktop khi Engine đang tắt.

## Khôi phục trang cấu hình ESP32 sau khi chuyển thư mục - 06/10/2026

- Kiểm tra bản chạy trên Desktop: Docker/database/backend hoạt động; tài khoản ESP32 được chấp nhận qua TCP và WSS. `/status` trên ESP32 cũ trả HTTP 500. Bản sao NVS đọc qua COM3 xác nhận namespace `app_config` còn Wi-Fi và cờ cấu hình, thiếu `mqtt_uri`, `mqtt_user`, `mqtt_pass`; kiểm tra CRC các trang NVS đạt. Không kết luận xóa Docker là thao tác đã làm mất các trường này.
- Firmware đọc từng trường cho luồng khôi phục, giữ NVS, yêu cầu nhập lại mật khẩu khi bản ghi chưa đầy đủ. Trang hiển thị mã lỗi, tự thử lại khi phản hồi chậm/lỗi, cập nhật mã phiên và giữ nội dung đang nhập. Host tests bao phủ mất địa chỉ MQTT, thiếu mật khẩu, chuỗi quá dài, sai kiểu, lỗi truy cập bộ nhớ và khôi phục lưu cấu hình; các kiểm thử cảm biến/sự kiện/mạng đang có đều đạt.
- Đã sao lưu ứng dụng, NVS và bảng phân vùng; chỉ nạp ứng dụng tại `0x10000` qua COM3, giữ nguyên bootloader/NVS/bảng phân vùng. Chrome trên ESP32 thật xác nhận cảnh báo khôi phục, mật khẩu không trả ra biểu mẫu, dữ liệu nhập sai bị từ chối và Wi-Fi/WSS hợp lệ lưu thành công. Sau khởi động lại, cấu hình đọc bình thường; lưu cùng mạng/tài khoản với hai ô mật khẩu trống giữ kết nối và cấu hình sau lần khởi động tiếp theo.
- Bộ đếm số sự kiện tiếp theo trên thiết bị là 8 trong khi database đã có ID 1238. Lệnh bảo trì USB nâng lên 1239 khi MQTT chưa kết nối, giữ nguyên 7 bản ghi đang chờ. Kiểm thử xác nhận không hạ bộ đếm, lỗi ghi giữ số cũ và khởi động lại giữ số mới. Bảy bản ghi thật sau đó đã vào PostgreSQL, pendingEvents=0. Đồng hồ NTP đã đồng bộ; telemetry và đoạn AI có nhịp hợp lệ tiếp tục cập nhật.
- Script kiểm tra từ thư mục Desktop đạt demo local và WSS public nhận telemetry thật. Web public có Cloudflare Access nên cần đăng nhập trình duyệt; kiểm tra bằng script không coi bước đó đã hoàn tất. Mô hình AI vẫn chưa được huấn luyện. Nguồn sửa đã đồng bộ về cả thư mục E: và Desktop; báo cáo và firmware trong `demo-private/config-repair-20261006/`.
