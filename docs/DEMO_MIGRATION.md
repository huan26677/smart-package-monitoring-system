# Sao lưu và mang dự án sang máy demo mới

Hướng dẫn này dùng PowerShell trên Windows và Docker Desktop. Các lệnh chạy từ thư mục gốc có `docker-compose.yml`. Máy mới cần Docker Engine hoạt động và Internet để tải image/thư viện ở lần build đầu; không cần cài Java, Node.js, Maven hay PostgreSQL trên Windows. Chuẩn bị trước ngày demo.

**Có thể dùng hai script tự động với Python 3.10 trở lên:** máy cũ chạy `python Lenh/don_dep_dong_goi.py` để sao lưu rồi dọn các thành phần có thể tạo lại; chép thư mục đã dọn cùng `.env`/`demo-private` sang máy mới và chạy `python Lenh/chay_demo.py`. Script demo giữ dữ liệu đã có, chỉ khôi phục phần còn trống, kiểm tra dữ liệu ESP32 và mở dashboard. Các tùy chọn và cách hiểu kết quả có ở [README](../README.md#hai-lệnh-tự-động-để-đóng-gói-và-demo). Quy trình thủ công bên dưới dùng khi cần kiểm tra từng bước hoặc xử lý lỗi.

## 1. Những phần phải mang theo

| Phần | Cách mang theo | Nội dung |
| --- | --- | --- |
| Mã nguồn hiện tại | Chép thư mục dự án hoặc dùng Git với đủ thay đổi | Backend, dashboard, Compose, `emqx-docker/base.hocon`, firmware và tài liệu |
| `.env` | Chép riêng, không đưa lên Git | Tài khoản PostgreSQL, dashboard, MQTT backend và token tunnel |
| PostgreSQL | Tệp `smart-package-demo.dump` | Telemetry, sự kiện, Wi-Fi Anchor, các lần thu/nhãn AI và mô hình đã nạp nếu có |
| EMQX | Tệp gốc `emqx-export-*.tar.gz` | Cấu hình xác thực, tài khoản MQTT và quyền đang dùng |
| ESP32 đã nạp firmware | Mang nguyên thiết bị | Wi-Fi/MQTT và lịch sử sự kiện trong NVS |
| Tệp huấn luyện riêng nếu có | Chép riêng | Dataset JSON, báo cáo và mô hình Python đã xuất |

Docker image và volume không nằm trong mã nguồn. `.env`, `demo-private/`, thư viện tải về và tệp `*.local` bị Git bỏ qua. Nếu chỉ clone repository, phải mang thêm các phần riêng ở bảng trên và mọi thay đổi chưa commit. Không cần chép `node_modules`, `target` hay `firmware-esp32/build` để chạy demo bằng firmware đã nạp.

**Chỉ chuyển ESP32 sang mạng khác:** giữ máy chủ hiện tại và tunnel hoạt động, đổi mạng bằng [trang cấu hình thiết bị](MOBILE_NETWORK.md). Không phải khôi phục database hay chuyển token trong trường hợp này.

## 2. Sao lưu trên máy cũ

Trước lần sao lưu cuối để bàn giao, đợi `pendingEvents` về 0 rồi tắt nguồn ESP32 và dừng các thao tác thu dữ liệu/chỉnh sửa trên web. Giữ PostgreSQL và EMQX chạy để xuất dữ liệu. Bản xuất chỉ chứa dữ liệu đã nhận tại thời điểm sao lưu; nếu tiếp tục dùng máy cũ sau đó, cần xuất lại trước khi chuyển chính thức.

Tạo thư mục riêng và chép `.env` hiện tại:

```powershell
$demoBackup = Join-Path (Get-Location) ('demo-private\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $demoBackup -Force | Out-Null
Copy-Item -LiteralPath '.env' -Destination (Join-Path $demoBackup '.env')
```

Xuất PostgreSQL vào tệp trong container rồi dùng `docker cp` để giữ nguyên dữ liệu nhị phân. Không dùng `>` hoặc pipe nội dung dump ra PowerShell để tạo tệp:

```powershell
$dbUser = (docker exec smart-package-postgres printenv POSTGRES_USER).Trim()
$dbName = (docker exec smart-package-postgres printenv POSTGRES_DB).Trim()
docker exec smart-package-postgres pg_dump --username $dbUser --dbname $dbName --format=custom --file=/tmp/smart-package-demo.dump
if ($LASTEXITCODE -ne 0) { throw 'Sao lưu PostgreSQL thất bại' }
docker cp smart-package-postgres:/tmp/smart-package-demo.dump (Join-Path $demoBackup 'smart-package-demo.dump')
if ($LASTEXITCODE -ne 0) { throw 'Sao chép PostgreSQL thất bại' }
```

Xuất EMQX bằng công cụ của broker và giữ nguyên tên tệp được sinh ra:

```powershell
docker exec smart-package-emqx emqx ctl data export
if ($LASTEXITCODE -ne 0) { throw 'Sao lưu EMQX thất bại' }
$emqxExport = (docker exec smart-package-emqx sh -c 'ls -1t /opt/emqx/data/backup/emqx-export-*.tar.gz | head -n 1').Trim()
if ($LASTEXITCODE -ne 0 -or -not $emqxExport) { throw 'Không tìm thấy bản sao lưu EMQX' }
docker cp "smart-package-emqx:$emqxExport" $demoBackup
if ($LASTEXITCODE -ne 0) { throw 'Sao chép EMQX thất bại' }
Get-ChildItem -LiteralPath $demoBackup -Force | Select-Object Name,Length
```

Chép mã nguồn và cả thư mục `$demoBackup` sang máy mới bằng USB hoặc phương thức riêng phù hợp. Bộ sao lưu chứa thông tin truy cập; giữ trong `demo-private/`, không đưa vào repository hay báo cáo công khai. `.env` phải đi cùng bản xuất EMQX vì chứa mật khẩu backend tương ứng. Bộ sao lưu trước lần bổ sung tài khoản `smart-package-backend` không đủ cho cấu hình hiện tại.

EMQX hiện được cố định ở **6.3.1**. Dùng bản xuất/nhập chính thức để chuyển sang broker mới; không dùng một tệp tar của toàn bộ `/opt/emqx/data` thay cho bản `emqx ctl data export`. Giữ nguyên tên `emqx-export-*.tar.gz` khi nhập. Xem [tài liệu sao lưu EMQX](https://docs.emqx.com/en/emqx/latest/guides/backup-restore.html).

## 3. Khôi phục trên máy mới

Phần này dành cho **máy chưa chạy dự án và database đích còn trống**. Nếu máy đích đã có dữ liệu cần giữ, không khôi phục chồng lên; xuất và đánh giá dữ liệu đích trước. Các lệnh dưới đây không xóa database để làm lại.

### 3.1. Chọn bộ sao lưu và build

Mở PowerShell tại thư mục dự án đã chép sang máy mới. Thay đường dẫn ví dụ bằng thư mục sao lưu thực tế:

```powershell
Set-Location 'D:\smart-package-system'
$demoBackup = (Resolve-Path -LiteralPath '.\demo-private\THU_MUC_SAO_LUU').Path
Copy-Item -LiteralPath (Join-Path $demoBackup '.env') -Destination '.env'
docker compose config --quiet
if ($LASTEXITCODE -ne 0) { throw 'Kiểm tra lại .env và Compose' }
docker compose build
if ($LASTEXITCODE -ne 0) { throw 'Build thất bại' }
docker compose pull postgres cloudflared
if ($LASTEXITCODE -ne 0) { throw 'Tải image thất bại' }
docker compose up -d postgres emqx
if ($LASTEXITCODE -ne 0) { throw 'Khởi động PostgreSQL/EMQX thất bại' }
docker compose ps
```

Đợi **cả PostgreSQL và EMQX healthy**. Chưa chạy backend, web hoặc cloudflared: backend sẽ tự tạo bảng nếu được bật trước khi khôi phục. `.env` phải có cả `MQTT_USERNAME` và `MQTT_PASSWORD` ngay từ đầu vì Compose kiểm tra các biến bắt buộc dù chỉ chạy một phần dịch vụ. Kiểm tra thêm `docker exec smart-package-emqx emqx ctl status` báo node đã khởi động trước khi nhập bản sao lưu; nếu báo `not responding to pings`, chưa thực hiện bước nhập.

### 3.2. Khôi phục PostgreSQL

Kiểm tra database đích chưa có bảng của ứng dụng:

```powershell
$dbUser = (docker exec smart-package-postgres printenv POSTGRES_USER).Trim()
$dbName = (docker exec smart-package-postgres printenv POSTGRES_DB).Trim()
$tableCount = docker exec smart-package-postgres psql --username $dbUser --dbname $dbName -Atc "SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE';"
if ($LASTEXITCODE -ne 0 -or $tableCount.Trim() -ne '0') { throw 'Database đích chưa trống hoặc không kiểm tra được; dừng khôi phục' }
docker cp (Join-Path $demoBackup 'smart-package-demo.dump') smart-package-postgres:/tmp/smart-package-demo.dump
if ($LASTEXITCODE -ne 0) { throw 'Sao chép dump thất bại' }
docker exec smart-package-postgres pg_restore --username $dbUser --dbname $dbName --no-owner --no-acl --single-transaction --exit-on-error /tmp/smart-package-demo.dump
if ($LASTEXITCODE -ne 0) { throw 'Khôi phục PostgreSQL thất bại; chưa bật backend' }
```

`--single-transaction` giúp toàn bộ lần khôi phục hoàn tất hoặc được hoàn tác nếu có lỗi; `--no-owner --no-acl` cho phép người dùng PostgreSQL ở máy đích sở hữu các bảng. Xem [tài liệu pg_restore của PostgreSQL 17](https://www.postgresql.org/docs/17/app-pgrestore.html).

### 3.3. Khôi phục EMQX

Chọn đúng một bản xuất trong thư mục vừa mang sang; không đổi tên tệp:

```powershell
$emqxArchives = @(Get-ChildItem -LiteralPath $demoBackup -Filter 'emqx-export-*.tar.gz')
if ($emqxArchives.Count -ne 1) { throw 'Thư mục phải chứa đúng một bản xuất EMQX' }
$emqxArchive = $emqxArchives[0]
docker cp $emqxArchive.FullName "smart-package-emqx:/tmp/$($emqxArchive.Name)"
if ($LASTEXITCODE -ne 0) { throw 'Sao chép bản xuất EMQX thất bại' }
docker exec smart-package-emqx emqx ctl data import "/tmp/$($emqxArchive.Name)"
if ($LASTEXITCODE -ne 0) { throw 'Khôi phục EMQX thất bại; chưa bật backend' }
```

Lệnh nhập phải báo thành công. Giữ cùng phiên bản và cấu hình bảo mật như máy cũ; nếu công cụ báo không tương thích, dừng và kiểm tra thay vì tự bỏ qua xác minh. Tài khoản quản trị **EMQX Dashboard** cũng có thể được khôi phục; đây là tài khoản riêng, không dùng mật khẩu đăng nhập web trong `.env` để suy ra.

### 3.4. Bật backend và web, kiểm tra local

```powershell
docker compose up -d backend web
if ($LASTEXITCODE -ne 0) { throw 'Khởi động backend/web thất bại' }
docker compose ps
Invoke-RestMethod http://localhost:8088/api/health/database
Invoke-RestMethod http://localhost:8088/api/health/mqtt
```

Cả hai endpoint phải trả `status = OK`. Mở **http://localhost:8088**, đăng nhập bằng tài khoản dashboard trong `.env`, kiểm tra lịch sử cũ, các lần thu/nhãn và Wi-Fi Anchor đã được mang sang. ESP32 chưa phải online ở bước này vì chưa bàn giao tunnel.

Nếu log backend báo `Not authorized to connect`, kiểm tra bản xuất EMQX có người dùng `smart-package-backend` và mật khẩu có khớp `.env` đi cùng không. Không tắt xác thực để làm backend healthy. Sau khi sửa biến môi trường, dùng `docker compose up -d backend` để áp dụng, không chỉ `restart`.

## 4. Bàn giao tunnel và thử public

Public hostname được quản lý trên Cloudflare, không tự tạo khi clone source. Với tunnel đã dùng trong dự án, xác nhận hai tuyến:

| Hostname | Dịch vụ đích trong mạng Compose |
| --- | --- |
| `monitor.huan2k5.id.vn` | `http://web:80` |
| `mqtt.huan2k5.id.vn` | `http://emqx:8083` |

ESP32 dùng **wss://mqtt.huan2k5.id.vn/mqtt**, tài khoản **esp32-001** và mật khẩu đã lưu trong NVS. Hostname MQTT phải cho phép mở WebSocket mà không cần màn hình đăng nhập Cloudflare Access. Dashboard luôn yêu cầu đăng nhập ứng dụng; nếu hostname web có thêm Access, đăng nhập theo cấu hình đó.

Trên **máy cũ**, dừng connector để hai máy không phục vụ hai database/broker độc lập qua cùng hostname:

Cloudflare hỗ trợ nhiều connector cho cùng tunnel, nhưng hai bản triển khai riêng của dự án không chia sẻ database/broker. Yêu cầu chỉ để một máy phục vụ public ở đây xuất phát từ cách triển khai đó; xem [cơ chế replica của Cloudflare](https://developers.cloudflare.com/cloudflare-one/networks/connectors/cloudflare-tunnel/configure-tunnels/tunnel-availability/).

```powershell
docker compose stop cloudflared
```

Sau đó trên **máy mới**:

```powershell
docker compose up -d cloudflared
docker compose logs --tail 50 cloudflared
```

Mở **https://monitor.huan2k5.id.vn**, đăng nhập lại rồi bật ESP32. Để bộ mạch nằm yên trong lúc hiệu chuẩn. Kiểm tra **Đã kết nối**, thời điểm nhận dữ liệu mới liên tục cập nhật và `pendingEvents` về 0. Tạo một thao tác thử nhẹ, xác nhận sự kiện mới xuất hiện trong lịch sử database.

Khi đổi Wi-Fi, giữ **BOOT khoảng 3 giây rồi thả**, kết nối **SMART_PACKAGE_SETUP / smart1234**, mở **http://192.168.4.1** và lưu Wi-Fi 2,4 GHz có Internet. Giữ URI/tài khoản MQTT và để trống mật khẩu MQTT nếu giữ nguyên tài khoản. Không phải flash lại chỉ để đổi mạng; xem [MOBILE_NETWORK.md](MOBILE_NETWORK.md).

Nếu trang cấu hình báo cấu hình cũ thiếu hoặc sai định dạng, nhập lại cả hai mật khẩu theo thông báo. Firmware hỗ trợ khôi phục cho phép lưu cấu hình mới mà vẫn giữ lịch sử sự kiện. Với firmware cũ còn báo HTTP 500 và khóa nút lưu, cần nâng cấp phần ứng dụng một lần qua COM, giữ NVS. Việc xóa/tạo lại Docker không tự sửa cấu hình nằm trên ESP32.

## 5. Cài mới nếu không giữ database cũ

1. Sao chép `.env.example` thành `.env`; điền đủ biến, gồm mật khẩu dashboard ít nhất 16 ký tự, tài khoản/mật khẩu MQTT backend và token tunnel nếu demo public.
2. Chạy `docker compose up -d --build postgres emqx`, đợi healthy, mở **http://localhost:18083** và thiết lập đăng nhập quản trị EMQX.
3. Trong EMQX, cấu hình xác thực **Password-Based / Built-in Database** cho MQTT. Tạo người dùng backend khớp `MQTT_USERNAME`/`MQTT_PASSWORD` và người dùng `esp32-001` khớp mật khẩu đang lưu trên ESP32; không nhầm với tài khoản quản trị broker hay dashboard web.
4. Cấu hình quyền topic theo bảng bên dưới, kiểm tra TCP 1883 và WebSocket 8083 đều dùng xác thực; base HOCON không tự tạo người dùng MQTT.
5. Chạy `docker compose up -d backend web`, kiểm tra health rồi thực hiện bàn giao tunnel như mục 4.

| Người dùng | Quyền gửi | Quyền đăng ký nhận |
| --- | --- | --- |
| Backend | `smart-package/+/event-ack`, `smart-package/+/motion-ack`, `smart-package/+/motion-control` | `smart-package/+/telemetry`, `smart-package/+/event`, `smart-package/+/location-scan`, `smart-package/+/motion-window` |
| `esp32-001` | `smart-package/esp32-001/telemetry`, `smart-package/esp32-001/event`, `smart-package/esp32-001/location-scan`, `smart-package/esp32-001/motion-window` | `smart-package/esp32-001/event-ack`, `smart-package/esp32-001/motion-ack`, `smart-package/esp32-001/motion-control` |

Không cần superuser cho hai tài khoản MQTT. Cài mới có lịch sử trống: các sự kiện đã được ESP32 đánh dấu đồng bộ với database cũ sẽ không tự gửi lại vào database mới. Muốn giữ lịch sử và dữ liệu AI phải khôi phục PostgreSQL ở mục 3. Wi-Fi Anchor mới cần BSSID/tọa độ thật tại địa điểm mới; anchor mang từ nơi cũ không mặc nhiên xác định được vị trí mới.

## 6. Chạy demo sau khi đã chuẩn bị

```powershell
docker compose up -d
docker compose ps
Invoke-RestMethod http://localhost:8088/api/health/database
Invoke-RestMethod http://localhost:8088/api/health/mqtt
```

Bật ESP32, đăng nhập dashboard và theo kịch bản ở [README](../README.md#12-kịch-bản-demo-đề-xuất). Demo **Bình thường → Nghiêng → Rung lắc → Va đập**, xem lịch sử, thử mất mạng riêng trên ESP32 rồi kết nối lại. Với bộ mạch đang ở breadboard, chỉ gõ nhẹ cạnh nhựa bằng đầu ngón tay và giữ chắc bộ mạch, tránh linh kiện/chân nối; không cần thử rơi tự do để trình diễn.

Giới hạn cần trình bày đúng: NVS tối đa 128 sự kiện, hàng đợi RAM 32 bản ghi; không giữ toàn bộ dòng mẫu AI khi offline. AI hiện chưa có mô hình được huấn luyện/kiểm chứng trên kiện thật; demo được phần thu/gắn nhãn/xuất dữ liệu, cảnh báo đang dùng quy tắc ngưỡng. Dữ liệu không tự huấn luyện lại AI khi được lưu vào database.

Kết thúc dùng `docker compose stop` để giữ dữ liệu. Không dùng `docker compose down -v`, prune volume hoặc xóa NVS khi vẫn cần lịch sử. Lần sau chỉ cần `docker compose up -d` nếu image, volume và cấu hình còn nguyên.

## 7. Kiểm tra quy trình ngày 06/10/2026

Đã tạo bộ sao lưu mới tại `demo-private/20261006-160825/` và chạy nguyên các khối lệnh PowerShell khôi phục ở mục 3.2/3.3 trên PostgreSQL và EMQX riêng, dữ liệu đích trống và tên node EMQX khác máy chủ đang chạy. Database khôi phục có lịch sử telemetry/sự kiện, Wi-Fi Anchor và các lần thu/đoạn AI; cả hai người dùng MQTT được khôi phục. Tài khoản backend từ `.env` đi cùng được chấp nhận, mật khẩu sai bị từ chối. Chưa có mô hình AI đã nạp trong database nguồn.

Đây là kiểm tra khôi phục bằng container riêng trên máy hiện tại, chưa phải xác nhận chạy trên máy tính mới hay tại mạng của lớp. Khi mang máy mới đi demo, vẫn cần thực hiện mục 4 để kiểm tra public, ESP32 và mạng thực tế. Các container kiểm tra đã được xóa sau khi hoàn tất; máy chủ đang chạy không bị chuyển tunnel hoặc khôi phục chồng dữ liệu. Kết quả chi tiết ở `migration-verification.local.json` trong thư mục sao lưu; nếu nguồn có dữ liệu mới sau thời điểm này, xuất lại trước lần chuyển chính thức.
