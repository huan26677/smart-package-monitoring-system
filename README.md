# Smart Package Monitoring System

Hệ thống IoT giám sát va đập, trạng thái vận chuyển và vị trí tương đối của kiện hàng sử dụng ESP32-S3, cảm biến MPU6050/MPU6500-compatible, MQTT, Spring Boot, PostgreSQL và React.

Tài liệu này được viết theo mục tiêu **mang toàn bộ đồ án sang máy demo khác, lên lớp chỉ cần bật Docker, cấp nguồn ESP32 và trình diễn**.

**Máy mới cần được chuẩn bị và thử trước ngày demo.** Chép mã nguồn chưa mang theo Docker image, database hay tài khoản MQTT. Làm theo [hướng dẫn sao lưu và chuyển máy](docs/DEMO_MIGRATION.md) trước, rồi dùng các bước chạy/demo bên dưới. Nếu chỉ mang ESP32 sang mạng khác và vẫn giữ máy chủ cũ chạy, xem [đổi mạng cho thiết bị](docs/MOBILE_NETWORK.md).

### Hai lệnh tự động để đóng gói và demo

Cần **Python 3.10 trở lên** và Docker Desktop. Chạy tại thư mục gốc dự án:

```powershell
python Lenh/don_dep_dong_goi.py --xem-truoc
python Lenh/don_dep_dong_goi.py
```

Lệnh đầu chỉ xem dung lượng. Lệnh thứ hai tạo bộ sao lưu PostgreSQL/EMQX cùng cấu hình `.env` trong `demo-private/`, rồi xóa thư viện, môi trường Python và các tệp build có thể tạo lại. Bản sao lưu trong `firmware-esp32/build/backups` được chép sang `demo-private/firmware-backups/` trước khi dọn build. Mã nguồn, `.git`, `.env`, `sdkconfig`, dữ liệu AI và báo cáo được giữ. Docker phải chạy với PostgreSQL/EMQX đang hoạt động để xuất bản sao lưu mới; sao lưu thất bại thì không dọn. `--khong-sao-luu` chỉ dùng khi đã chủ động chuẩn bị dữ liệu riêng.

Chép hoặc nén thư mục đã dọn, **mang cả `.env` và `demo-private/`** sang máy mới. Bộ đóng gói chứa thông tin truy cập, không đưa lên GitHub. Trước khi bàn giao public, dừng `cloudflared` trên máy cũ theo mục 25.

Trên máy mới, chạy:

```powershell
python Lenh/chay_demo.py
```

Script tìm dự án theo vị trí của chính nó, thử mở Docker Desktop trên Windows nếu Engine chưa chạy, build các dịch vụ, tự khôi phục những phần còn trống từ bộ sao lưu đầy đủ mới nhất và mở dashboard. Script kiểm tra sức khỏe container, PostgreSQL/MQTT, xác thực MQTT, đăng nhập API, telemetry ESP32 thực sự cập nhật, hết sự kiện chờ đồng bộ và đoạn AI mới có nhịp hợp lệ; khi demo public còn kiểm tra HTTPS và WSS nhận telemetry thật. Không cần ESP32 cắm USB nếu thiết bị đang có nguồn và mạng.

Nếu web có Cloudflare Access, script báo cần đăng nhập trên trình duyệt và không tự nhận đã kiểm tra xong web public. LCD/còi cần quan sát bằng thao tác thử thật. Chưa có mô hình AI không ngăn demo quy tắc và thu dữ liệu; script ghi rõ tình trạng đó.

```powershell
python Lenh/chay_demo.py --chi-kiem-tra --khong-mo-trinh-duyet
python Lenh/chay_demo.py --khong-public
python Lenh/chay_demo.py --ban-sao-luu demo-private/THU_MUC_SAO_LUU --doi-esp32 120
```

`--chi-kiem-tra` không build/khởi động/khôi phục. `--khong-public` chỉ xác nhận demo local. Mã kết thúc: **0** = đạt các kiểm tra của chế độ đã chọn, **1** = có lỗi, **2** = local và WSS đạt nhưng web public còn cần đăng nhập/kiểm tra Cloudflare Access, **130** = người dùng dừng. Báo cáo không chứa mật khẩu lưu trong `demo-private/lenh/`. Script không khôi phục chồng database đã có bảng và không tự xóa Docker volume.

---

## 1. Kiến trúc hệ thống

```text
ESP32-S3
   │
   │ Wi-Fi 2.4 GHz / Hotspot
   │ MQTT over WSS
   ▼
mqtt.huan2k5.id.vn:443/mqtt
   │
   ▼
Cloudflare Tunnel
   │
   ▼
EMQX WebSocket :8083
   │
   ▼
Spring Boot
   │
   ├── PostgreSQL
   │
   └── REST API
          │
          ▼
       Nginx
          │
          ▼
    React Dashboard
          │
          ▼
https://monitor.huan2k5.id.vn
```

Các dịch vụ chạy bằng Docker Compose:

```text
smart-package-emqx
smart-package-postgres
smart-package-backend
smart-package-web
smart-package-cloudflared
```

Tên Docker project đã được cố định:

```text
smart-package-system
```

---

## 2. Phần cứng sử dụng

- ESP32-S3
- MPU6050 / MPU6500-compatible
- LCD1602
- Active buzzer
- Wi-Fi 2.4 GHz
- Máy tính chạy Docker Desktop
- Điện thoại phát hotspot hoặc Wi-Fi có Internet

Các trạng thái kiện hàng:

```text
NORMAL
VIBRATION
TILT
FLIP
FREE_FALL
IMPACT
DROP
```

---

## 3. Chuẩn bị máy demo

Phần này chỉ cần thực hiện **một lần trước ngày demo**.

Máy demo cần có:

- Windows 10/11
- Docker Desktop
- Docker Compose
- thư mục dự án đầy đủ
- file `.env`
- PostgreSQL volume đã được khôi phục nếu muốn giữ dữ liệu cũ
- EMQX volume đã được khôi phục để giữ tài khoản MQTT
- Internet hoạt động

Không cần cài Java, Node.js, PostgreSQL, EMQX, Maven hoặc npm vì chúng chạy trong Docker.

Kiểm tra Docker:

```powershell
docker --version
docker compose version
```

### Chuyển từ máy hiện tại sang máy mới

1. Trên máy cũ, xuất PostgreSQL bằng `pg_dump`, xuất cấu hình và tài khoản EMQX bằng `emqx ctl data export`, sao chép `.env` cùng thời điểm.
2. Chép mã nguồn hiện tại và bộ sao lưu riêng sang máy mới. Các thay đổi chưa commit cũng phải được mang theo; `git clone` không tự lấy chúng. `.env` và `demo-private/` không nằm trong Git.
3. Trên máy mới, đặt `.env` ở thư mục gốc, build image, chạy **chỉ PostgreSQL và EMQX** rồi khôi phục dữ liệu trước khi bật backend/web.
4. Kiểm tra tài khoản MQTT của backend và ESP32 đã được khôi phục, chạy backend/web và kiểm tra hai endpoint health đều `OK`.
5. Dừng Cloudflare Tunnel trên máy cũ, sau đó bật tunnel trên máy mới và kiểm tra hai hostname public.

Các lệnh PowerShell, thứ tự thực hiện và cách cài mới không dùng dữ liệu cũ có trong [DEMO_MIGRATION.md](docs/DEMO_MIGRATION.md). Khi chuyển chính thức, dùng bộ sao lưu mới; bản sao lưu tạo trước lần bổ sung tài khoản MQTT backend không đủ cho cấu hình hiện tại.

---

## 4. File `.env`

Ở thư mục gốc dự án phải có:

```text
.env
```

Ví dụ cấu trúc:

```env
POSTGRES_DB=smart_package
POSTGRES_USER=smartpackage
POSTGRES_PASSWORD=MAT_KHAU_POSTGRES

MQTT_USERNAME=smart-package-backend
MQTT_PASSWORD=MAT_KHAU_MQTT_BACKEND

DASHBOARD_ADMIN_USERNAME=admin
DASHBOARD_ADMIN_PASSWORD=MAT_KHAU_DASHBOARD_IT_NHAT_16_KY_TU

CLOUDFLARE_TUNNEL_TOKEN=TOKEN_CLOUDFLARE
```

Không đưa `.env` lên GitHub.

`MQTT_USERNAME`/`MQTT_PASSWORD` là **tài khoản MQTT của backend**, phải tồn tại trong EMQX với đúng mật khẩu. Đây là tài khoản riêng với `esp32-001` trên ESP32 và tài khoản đăng nhập dashboard. Hai biến MQTT là bắt buộc trong Compose hiện tại; chỉ chép `.env` không tự tạo tài khoản trong EMQX. Khi khôi phục, dùng `.env` đi cùng bản xuất EMQX. Các giá trị trên chỉ là chỗ điền, không dùng làm mật khẩu thật.

---

## 5. Cách chạy hệ thống trong ngày demo

### Bước 1: Bật Docker Desktop

Chờ Docker Engine chạy xong.

### Bước 2: Mở PowerShell tại thư mục dự án

Ví dụ:

```powershell
cd D:\smart-package-system
```

### Bước 3: Chạy toàn bộ hệ thống

```powershell
docker compose up -d
```

Đây là lệnh chính dùng trong ngày demo.

Lệnh này áp dụng sau khi máy mới đã build image, khôi phục dữ liệu và thử kết nối thành công theo mục 3.

Nếu vừa sửa source và cần build lại image:

```powershell
docker compose up -d --build
```

Không cần `--build` mỗi lần nếu source không thay đổi.

### Bước 4: Kiểm tra container

```powershell
docker compose ps
```

Trạng thái mong đợi:

```text
smart-package-emqx          Up / healthy
smart-package-postgres      Up / healthy
smart-package-backend       Up / healthy
smart-package-web           Up
smart-package-cloudflared   Up
```

---

## 6. Kiểm tra nhanh trước khi trình bày

### Kiểm tra database

```powershell
Invoke-RestMethod http://localhost:8088/api/health/database
```

Mong đợi:

```text
status = OK
```

### Kiểm tra MQTT backend

```powershell
Invoke-RestMethod http://localhost:8088/api/health/mqtt
```

Mong đợi:

```text
status = OK
```

### Kiểm tra log

```powershell
docker compose logs backend --tail=100
docker compose logs cloudflared --tail=100
```

---

## 7. Mở Web Dashboard

Local:

```text
http://localhost:8088
```

Public:

```text
https://monitor.huan2k5.id.vn
```

Dashboard sử dụng đăng nhập của ứng dụng Spring Security.

Thông tin đăng nhập lấy từ:

```text
DASHBOARD_ADMIN_USERNAME
DASHBOARD_ADMIN_PASSWORD
```

trong `.env`.

Cloudflare Access không bắt buộc trong cấu hình demo hiện tại.

---

## 8. Chuẩn bị ESP32

ESP32 đã được nạp firmware trước khi demo.

Không cần build hoặc flash lại firmware trong ngày demo nếu firmware đang hoạt động.

Broker Internet:

```text
wss://mqtt.huan2k5.id.vn/mqtt
```

ESP32 kết nối bằng WSS cổng 443 nên không phụ thuộc IP LAN của máy demo.

Máy chạy Docker phải bật và có Internet trong suốt buổi trình diễn. Nếu chuyển toàn bộ máy chủ, tunnel trên máy mới phải dùng hostname `monitor.huan2k5.id.vn` → `http://web:80` và `mqtt.huan2k5.id.vn` → `http://emqx:8083`. ESP32 không có modem SIM; khi di chuyển cần Wi-Fi/điểm phát sóng trong phạm vi kết nối.

---

## 9. Cách đổi Wi-Fi cho ESP32 khi tới lớp

Nếu ESP32 đã lưu đúng hotspot dùng để demo thì chỉ cần cấp nguồn.

Nếu cần đổi Wi-Fi:

1. Bật ESP32 bình thường.
2. Giữ nút **BOOT khoảng 3 giây rồi thả**.
3. Kết nối vào:

```text
SSID: SMART_PACKAGE_SETUP
Password: smart1234
```

4. Mở:

```text
http://192.168.4.1
```

5. Nhập Wi-Fi hoặc hotspot mới.
6. Ưu tiên Wi-Fi 2.4 GHz có Internet.
7. Giữ nguyên MQTT:

```text
wss://mqtt.huan2k5.id.vn/mqtt
```

8. Giữ nguyên MQTT username:

```text
esp32-001
```

9. Nếu tài khoản MQTT không đổi thì có thể để trống mật khẩu MQTT để giữ mật khẩu đã lưu.
10. Bấm **Lưu và kết nối**.

ESP32 lưu cấu hình vào NVS và khởi động lại.

Không cần sửa code và không cần flash lại firmware.

---

## 10. ESP32 tự phục hồi khi mất Wi-Fi

Firmware tự thử kết nối lại khoảng mỗi:

```text
5 giây
```

Sau khoảng:

```text
60 giây
```

chưa nhận được IP, ESP32 tự mở:

```text
SMART_PACKAGE_SETUP
```

Trong thời gian mất mạng:

- cảm biến vẫn hoạt động
- LCD vẫn hoạt động
- buzzer vẫn hoạt động
- phát hiện sự kiện vẫn hoạt động
- event tiếp tục được lưu vào NVS
- event chưa đồng bộ sẽ được gửi lại khi mạng trở lại

Event chỉ được đánh dấu đã đồng bộ sau khi backend xác nhận đã lưu vào PostgreSQL.

NVS giữ tối đa **128 sự kiện**, hàng đợi trước khi ghi flash giữ **32 bản ghi trong RAM**. Khi đầy toàn bộ bản ghi chưa đồng bộ, sự kiện mới bị từ chối; mất nguồn có thể mất bản ghi còn trong RAM. Toàn bộ dòng cảm biến và các đoạn AI không được lưu ngoại tuyến để gửi bù đầy đủ.

---

## 11. Kiểm tra ESP32 đã kết nối

Sau khi cấp nguồn ESP32, mở Dashboard.

Thiết bị:

```text
esp32-001
```

phải chuyển sang:

```text
ONLINE
```

Telemetry cần tiếp tục thay đổi:

```text
Total G
Angle
Vibration
Wi-Fi RSSI
State
```

Trên giao diện tiếng Việt, kiểm tra **Đã kết nối**, **Gia tốc tổng hợp**, **Góc nghiêng**, **Mức rung**, **Cường độ Wi-Fi** và thời điểm nhận dữ liệu mới. Giá trị đứng yên có thể gần như không đổi; cần theo dõi thời điểm nhận. Khi mất kết nối, giao diện ghi **Dữ liệu cũ** và hiển thị thời điểm nhận cuối, không coi nhãn Bình thường cũ là trạng thái hiện tại.

Nếu ESP32 chưa ONLINE:

```powershell
docker compose ps
docker compose logs emqx --tail=100
docker compose logs backend --tail=100
docker compose logs cloudflared --tail=100
```

---

## 12. Kịch bản demo đề xuất

Thời lượng phù hợp khoảng **6 đến 8 phút**.

### Phần 1: Giới thiệu

Mở:

```text
https://monitor.huan2k5.id.vn
```

Giới thiệu ngắn:

> Hệ thống sử dụng ESP32-S3 và cảm biến chuyển động để giám sát tình trạng kiện hàng. ESP32 xử lý dữ liệu tại thiết bị, truyền dữ liệu bằng MQTT qua Internet đến EMQX, backend Spring Boot lưu dữ liệu vào PostgreSQL và Web Dashboard hiển thị trạng thái theo thời gian thực.

Kiến trúc:

```text
ESP32
→ Internet
→ Cloudflare Tunnel
→ EMQX
→ Spring Boot
→ PostgreSQL
→ React Dashboard
```

### Phần 2: Trạng thái thời gian thực

Cho thấy:

```text
Device: esp32-001
Status: ONLINE
```

Trình bày:

- Total G
- góc nghiêng
- độ rung
- RSSI Wi-Fi
- trạng thái kiện hàng

### Phần 3: Demo nghiêng

Nghiêng kiện từ từ.

Dashboard:

```text
NORMAL
→ TILT
```

LCD cũng hiển thị cảnh báo nghiêng.

### Phần 4: Demo rung

Rung kiện nhẹ có kiểm soát.

Dashboard có thể hiển thị:

```text
VIBRATION
```

và giá trị VibRMS thay đổi.

### Phần 5: Demo va đập

Tạo va đập có kiểm soát.

Hệ thống:

```text
phát hiện IMPACT
→ phân mức
→ buzzer cảnh báo
→ LCD hiển thị
→ lưu event vào NVS
→ gửi MQTT
→ backend lưu PostgreSQL
→ backend gửi ACK
→ ESP đánh dấu synced
```

Mở Event History để cho thấy:

```text
Type
Level
Peak G
Angle
Vibration
Duration
Time
```

---

## 13. Demo khả năng hoạt động khi mất mạng

Đây là phần nên trình diễn vì thể hiện độ tin cậy của hệ thống.

1. ESP32 đang ONLINE.
2. Tắt hotspot hoặc làm ESP32 mất Wi-Fi.
3. Dashboard chuyển OFFLINE sau timeout.
4. Trong lúc offline, tạo một va đập có kiểm soát.
5. ESP32 vẫn phát hiện, cảnh báo và lưu event vào NVS.
6. Bật Wi-Fi/hotspot trở lại.
7. ESP32 tự reconnect.
8. Event chưa đồng bộ được gửi lại.
9. Backend lưu PostgreSQL rồi gửi `event-ack`.
10. Theo dõi:

```text
pendingEvents → 0
```

Giữ máy demo có Internet và dashboard đang truy cập được trong phép thử này. Nếu máy demo cũng dùng chính hotspot vừa tắt, nên ngắt riêng Wi-Fi của ESP32 hoặc dùng một mạng khác cho máy demo. Thiết bị được coi là mất kết nối sau hơn 15 giây không có dữ liệu mới; dashboard kiểm tra lại mỗi 3 giây.

`rejectedEvents` là bộ đếm lịch sử các lần không lưu được, không tự về 0 khi mạng trở lại. Cảnh báo này khác với số sự kiện đang chờ đồng bộ `pendingEvents`.

Giải thích:

> QoS 1 chỉ xác nhận broker đã nhận MQTT. Hệ thống dùng thêm ACK của backend để đảm bảo event chỉ được đánh dấu đồng bộ sau khi database đã lưu thành công.

---

## 14. Demo Wi-Fi Anchor và vị trí tương đối

Hệ thống không dùng GPS.

ESP32 quét:

```text
BSSID
RSSI
```

Backend so sánh BSSID với Wi-Fi Anchor đã đăng ký.

Nếu match:

```text
ANCHOR_MATCHED
```

Dashboard hiển thị:

- tên Anchor
- BSSID
- RSSI
- latitude
- longitude
- vị trí trên OpenStreetMap

Nếu không match:

```text
NO_ANCHOR
```

Hệ thống không tạo tọa độ giả.

Ở địa điểm mới có thể tạo thêm Anchor mới mà không cần xóa Anchor cũ.

---

## 15. Demo lịch sử vị trí

Dashboard hiển thị:

- thời gian
- trạng thái
- Anchor
- BSSID
- RSSI
- latitude
- longitude

Lưu ý: đây là **vị trí tương đối theo Wi-Fi Anchor**, không phải GPS tracking liên tục.

---

## 16. Phần AI

Pipeline hiện có:

```text
ESP32
→ thu cửa sổ cảm biến 200 mẫu
→ backend lưu dữ liệu
→ gắn nhãn
→ xuất dataset
→ Python Random Forest
→ đánh giá
→ export model
→ Java inference
```

Nếu chưa có model được huấn luyện bằng dữ liệu kiện hàng thật thì chỉ trình bày đây là phần thực nghiệm đang tiếp tục.

Hiện chưa có mô hình được huấn luyện và kiểm chứng trên kiện hàng thật. Demo được phần nhận đoạn cảm biến, thu dữ liệu, xem đồ thị, gắn nhãn và xuất dữ liệu. Các cảnh báo va đập đang hoạt động theo ngưỡng riêng, không phải kết quả AI. Chỉ các đoạn của lần thu được lưu lâu dài trong database; dòng phân tích trực tiếp không liên tục ghi toàn bộ mẫu thô. Dữ liệu mới chỉ giúp cải thiện AI sau khi được gắn nhãn, đủ điều kiện huấn luyện/đánh giá và nạp mô hình mới; hệ thống không tự học từ mọi sự kiện.

Không nên tuyên bố độ chính xác nếu chưa có đủ dữ liệu thực nghiệm thật.

Xem:

```text
docs/AI_GUIDE.md
docs/EXPERIMENTS.md
```

---

## 17. Các URL quan trọng

```text
Dashboard public:
https://monitor.huan2k5.id.vn

Dashboard local:
http://localhost:8088

Backend local:
http://localhost:8080

EMQX Dashboard:
http://localhost:18083

MQTT Internet:
wss://mqtt.huan2k5.id.vn/mqtt

ESP32 Setup Portal:
http://192.168.4.1

Setup Wi-Fi:
SMART_PACKAGE_SETUP
Password: smart1234
```

---

## 18. Các lệnh dùng nhiều nhất

Khởi động:

```powershell
docker compose up -d
```

Kiểm tra:

```powershell
docker compose ps
```

Xem log:

```powershell
docker compose logs --tail=100
```

Backend:

```powershell
docker compose logs backend --tail=100
```

EMQX:

```powershell
docker compose logs emqx --tail=100
```

Web:

```powershell
docker compose logs web --tail=100
```

Cloudflare:

```powershell
docker compose logs cloudflared --tail=100
```

Theo dõi backend realtime:

```powershell
docker compose logs -f backend
```

Thoát log bằng `Ctrl + C`. Container vẫn chạy.

---

## 19. Khởi động lại service khi cần

```powershell
docker compose restart backend
docker compose restart emqx
docker compose restart web
docker compose restart cloudflared
```

Toàn bộ:

```powershell
docker compose restart
```

---

## 20. Nếu Public Web không vào được

Kiểm tra local trước:

```text
http://localhost:8088
```

Nếu local chạy nhưng domain không chạy:

```powershell
docker compose logs cloudflared --tail=100
```

Kiểm tra máy demo có Internet và `smart-package-cloudflared` đang Up.

Không cần mở port modem/router.

---

## 21. Nếu Web chạy nhưng ESP32 OFFLINE

Nếu banner báo **Máy chủ mất kết nối MQTT**, kiểm tra `/api/health/mqtt` và tài khoản `MQTT_USERNAME`/`MQTT_PASSWORD` của backend trước. Log `Not authorized to connect` thường là tài khoản chưa được khôi phục hoặc mật khẩu không khớp. Sau khi sửa `.env`, chạy `docker compose up -d backend`; `restart` không áp dụng biến môi trường mới.

Nếu MQTT health đã `OK` nhưng banner báo **Thiết bị mất kết nối**, kiểm tra đường truyền và tài khoản của ESP32 theo các bước bên dưới. Nếu banner báo **Chưa cập nhật được dữ liệu**, kiểm tra API/backend và tải lại trang.

Kiểm tra:

```powershell
docker compose logs emqx --tail=100
docker compose logs backend --tail=100
docker compose logs cloudflared --tail=100
```

Broker trên ESP phải là:

```text
wss://mqtt.huan2k5.id.vn/mqtt
```

Không dùng IP LAN máy demo.

Kiểm tra:

- Wi-Fi có Internet
- Wi-Fi 2.4 GHz
- MQTT username đúng
- MQTT password đúng
- Cloudflare Tunnel đang chạy
- EMQX healthy

---

## 22. Nếu Docker có service không healthy

```powershell
docker compose ps
```

PostgreSQL:

```powershell
docker compose logs postgres --tail=100
```

EMQX:

```powershell
docker compose logs emqx --tail=100
```

Backend:

```powershell
docker compose logs backend --tail=150
```

---

## 23. Nếu vừa thay đổi code

```powershell
docker compose up -d --build
```

Nếu không sửa code, ngày demo chỉ cần:

```powershell
docker compose up -d
```

---

## 24. Những lệnh không được chạy trong ngày demo

Không chạy:

```powershell
docker compose down -v
```

vì `-v` xóa Docker volume, bao gồm PostgreSQL và dữ liệu persistent của EMQX.

Không chạy:

```powershell
docker volume prune
docker system prune --volumes
```

Không chạy:

```powershell
idf.py erase-flash
```

nếu không muốn xóa NVS của ESP32.

---

## 25. Không chạy Cloudflare Tunnel trên hai máy demo cùng lúc

Nếu máy server cũ vẫn đang chạy cùng tunnel, dừng cloudflared trên máy đó:

```powershell
docker compose stop cloudflared
```

hoặc:

```powershell
docker compose stop
```

Trong buổi demo chỉ nên để máy demo chạy tunnel.

Cloudflare hỗ trợ nhiều connector cho cùng tunnel, nhưng dự án này dùng database và broker riêng trên từng máy. Để cả hai máy chạy cùng token có thể khiến web và ESP32 đi tới các bộ dữ liệu khác nhau. Đây là yêu cầu cho cách demo của dự án; xem [quy trình bàn giao tunnel](docs/DEMO_MIGRATION.md#4-bàn-giao-tunnel-và-thử-public).

---

## 26. Checklist trước khi rời nhà

- [ ] Docker Desktop mở được.
- [ ] `.env` nằm đúng thư mục gốc.
- [ ] `.env` có tài khoản MQTT backend khớp EMQX.
- [ ] Máy mới đã build image và khôi phục theo [hướng dẫn chuyển máy](docs/DEMO_MIGRATION.md).
- [ ] PostgreSQL volume có dữ liệu.
- [ ] EMQX volume có tài khoản MQTT.
- [ ] `docker compose up -d` chạy thành công.
- [ ] `docker compose ps` không có container lỗi.
- [ ] Database health = OK.
- [ ] MQTT health = OK.
- [ ] `http://localhost:8088` mở được.
- [ ] `https://monitor.huan2k5.id.vn` mở được.
- [ ] Đăng nhập Dashboard được.
- [ ] ESP32 lên ONLINE.
- [ ] Telemetry thay đổi.
- [ ] Tạo một IMPACT thử và event xuất hiện.
- [ ] `pendingEvents` về 0.
- [ ] ESP32 đã lưu hotspot dùng ở lớp.
- [ ] Hotspot điện thoại đặt 2.4 GHz.
- [ ] Điện thoại còn pin và data.
- [ ] Mang dây nguồn ESP32.
- [ ] Mang dây USB dự phòng.
- [ ] Máy server cũ không chạy cùng Cloudflare Tunnel.
- [ ] Có bộ sao lưu mới gồm PostgreSQL, bản xuất EMQX và `.env` cùng thời điểm.

---

## 27. Checklist ngay trước khi thuyết trình

```powershell
docker compose up -d
docker compose ps
```

Sau đó:

```powershell
Invoke-RestMethod http://localhost:8088/api/health/database
Invoke-RestMethod http://localhost:8088/api/health/mqtt
```

Mở:

```text
https://monitor.huan2k5.id.vn
```

Bật ESP32.

Đợi:

```text
esp32-001 → ONLINE
```

Đặt kiện ở tư thế chuẩn và để yên trong quá trình calibration ban đầu.

Sau đó mới bắt đầu demo.

---

## 28. Kết thúc demo

Dừng hệ thống:

```powershell
docker compose stop
```

Dữ liệu PostgreSQL và EMQX volume vẫn được giữ.

Lần sau:

```powershell
docker compose up -d
```

Không cần restore lại nếu volume vẫn còn.

---

## 29. Tài liệu liên quan

- [Sao lưu, chuyển máy và chuẩn bị demo](docs/DEMO_MIGRATION.md)
- [Vận hành và tài khoản MQTT](docs/RUNNING.md)
- [Đổi mạng cho ESP32](docs/MOBILE_NETWORK.md)
- [Tình trạng dự án](docs/PROJECT_STATUS.md)
- [Kiểm thử](docs/TESTING.md)
- [Kế hoạch thực nghiệm](docs/EXPERIMENTS.md)
- [Thu dữ liệu và AI](docs/AI_GUIDE.md)

---

## 30. Quy trình demo ngắn gọn nhất

```text
1. Bật hotspot/Wi-Fi có Internet
2. Bật Docker Desktop
3. Mở PowerShell tại thư mục dự án
4. docker compose up -d
5. docker compose ps
6. Bật ESP32
7. Mở https://monitor.huan2k5.id.vn
8. Đăng nhập
9. Chờ esp32-001 ONLINE
10. Demo NORMAL → TILT → VIBRATION → IMPACT
11. Mở Event History
12. Demo mất mạng → lưu NVS → kết nối lại → pendingEvents về 0
13. Demo Wi-Fi Anchor / bản đồ
14. Trình bày phần AI nếu có dữ liệu thực nghiệm
```

Mục tiêu của bản triển khai này là: **không cần sửa code, không cần flash lại, không cần cài Java/Node/PostgreSQL thủ công và không cần cấu hình IP LAN khi mang dự án sang máy hoặc mạng khác.**

Điều kiện: đã chuẩn bị Docker/image và khôi phục dữ liệu, tài khoản, tunnel trên máy mới; máy chủ đang chạy và ESP32 có Wi-Fi 2,4 GHz kết nối Internet.
