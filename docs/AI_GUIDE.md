# Thu dữ liệu và sử dụng AI phân biệt va đập

AI có ba nhãn **Bình thường / Rung lắc / Va đập**, chạy trên máy chủ bằng Random Forest. ESP32 gửi một đoạn gồm 200 mẫu gia tốc và tốc độ góc ba trục, khoảng hai giây. Các cảnh báo theo ngưỡng, còi và lịch sử sự kiện tiếp tục hoạt động độc lập.

Hiện chưa có mô hình được huấn luyện bằng dữ liệu kiện hàng thật. Màn hình hiển thị **Chưa huấn luyện**, không tạo kết quả hay độ chính xác giả.

## 1. Chuẩn bị

- Mở dashboard tại http://localhost:8088 và đăng nhập bằng tài khoản hiện tại.
- Chọn `esp32-001`, xem phần **AI phân biệt va đập**. Phải có giờ nhận mới và nút **Bắt đầu thu** được bật.
- Cố định cảm biến vào kiện mẫu. Sau khi bật ESP32, giữ yên trong lúc lấy tư thế chuẩn.
- Ghi lại loại kiện, khối lượng, cách đóng gói, vị trí cảm biến và thao tác ở từng buổi. Nhãn là thao tác quan sát, không phải kết quả theo ngưỡng hay AI.

## 2. Thu và gắn nhãn

1. Nhập **Tên lần thu**, ví dụ “Mang kiện nhẹ nhàng”.
2. Nhập **Nhóm buổi thử**, ví dụ “Buổi 1 – kiện mẫu A”. Các lần thu liên quan trong cùng buổi/điều kiện phải giữ cùng nhóm. Chỉ đổi nhóm khi thực sự bắt đầu một buổi/lần thử độc lập.
3. Chọn thời gian 10–120 giây, bấm **Bắt đầu thu**, rồi thực hiện thao tác và ghi nhận thời điểm.
4. Chờ số đoạn nhận đủ. Mỗi đoạn khoảng hai giây; nút **Xem đoạn** mở đồ thị sáu trục.
5. Chọn nhãn quan sát cho từng đoạn:
   - **Bình thường**: đứng yên hoặc vận chuyển nhẹ nhàng, không có rung/va đập chủ ý.
   - **Rung lắc**: rung hoặc lắc liên tục, không có cú va đập trong đoạn.
   - **Va đập**: có một cú va đập đã quan sát trong đoạn, kể cả khi phần còn lại đứng yên.
6. Nếu không xác định được thao tác tương ứng hoặc đoạn nằm giữa hai thao tác, giữ **Chưa gắn nhãn**. Đồ thị giúp đối chiếu thời điểm; không tự dùng đỉnh gia tốc làm đáp án.

Cần ít nhất **30 đoạn đã gắn nhãn và 3 nhóm buổi thử độc lập cho mỗi loại**. Đây là điều kiện tối thiểu để thử huấn luyện, chưa chứng minh khả năng áp dụng rộng. Ví dụ trong mỗi buổi, thu khoảng 10 đoạn đủ điều kiện cho mỗi loại, rồi lặp ở ít nhất ba buổi độc lập; bổ sung nếu đoạn bị loại hoặc khó gắn nhãn. Dùng kiện mẫu và thao tác có kiểm soát theo [kế hoạch thực nghiệm](EXPERIMENTS.md).

Đoạn chạm giới hạn gia tốc ±16 g hoặc tốc độ góc ±500 độ/giây, có khoảng cách mẫu quá 30 ms hoặc thời lượng ngoài 1,75–2,5 giây sẽ không được xuất để huấn luyện. Gắn nhãn không làm đoạn lỗi trở thành hợp lệ.

## 3. Xuất và huấn luyện trên máy tính

Bấm **Xuất dữ liệu huấn luyện**. Tệp JSON gồm các đặc trưng, nhãn quan sát, nhóm buổi thử và kết quả quy tắc để so sánh. Các mẫu thô vẫn được lưu trong database để xem lại; đoạn chưa gắn nhãn hoặc chất lượng kém không nằm trong tệp xuất.

Từ thư mục gốc dự án, trên Windows:

```powershell
python -m venv ai-training/.venv.local
ai-training/.venv.local/Scripts/python.exe -m pip install -r ai-training/requirements.txt
ai-training/.venv.local/Scripts/python.exe ai-training/train.py "C:/duong-dan/du-lieu-ai-esp32-001.json" --output "ai-training/mo-hinh-ai.local.json"
```

Máy hiện tại đã có môi trường `ai-training/.venv.local` và các thư viện cần thiết. Chỉ cần chạy lệnh cuối với đường dẫn tệp vừa tải. Công cụ sẽ từ chối nếu chưa đủ dữ liệu; không sinh mô hình minh họa để thay thế.

Đầu ra gồm mô hình JSON và báo cáo `.bao-cao.txt`. Tên chỉ số và báo cáo đều bằng tiếng Việt. Các mã trường JSON giữ ổn định để firmware, Python và Java trao đổi dữ liệu.

## 4. Đọc báo cáo rồi nạp mô hình

- **Tỷ lệ đúng**: số đoạn dự đoán đúng trên toàn bộ tập kiểm tra.
- **Độ chính xác của một loại**: trong các đoạn được dự đoán thuộc loại đó, bao nhiêu đoạn đúng.
- **Tỷ lệ phát hiện**: trong các đoạn thật sự thuộc loại đó, bao nhiêu đoạn được nhận ra.
- **F1 trung bình**: trung bình F1 của ba loại; xem thêm F1 và tỷ lệ phát hiện của **Va đập**.
- **Tỷ lệ có kết luận**: số đoạn có điểm đủ ngưỡng, thay vì **Chưa chắc chắn**.
- **Bảng nhầm lẫn**: hàng là nhãn quan sát, cột là dự đoán. Chưa chắc chắn được tính là không nhận ra đúng.

Huấn luyện và kiểm tra tách theo nhóm buổi thử, không trộn các đoạn gần nhau của cùng buổi vào hai tập. Quy tắc hiện tại được đánh giá trên cùng các đoạn kiểm tra. Mô hình chỉ huấn luyện trên tập huấn luyện đã tách; không huấn luyện lại bằng cả tập rồi giữ nguyên báo cáo cũ.

Ngưỡng điểm mặc định 70%, chọn trước khi xem kết quả. Điểm dự đoán chưa được hiệu chuẩn thành xác suất chắc chắn. Nếu sửa ngưỡng/mô hình dựa trên báo cáo này, cần một tập buổi thử mới để báo cáo kết quả cuối.

Trong dashboard, chọn **Nạp mô hình đã huấn luyện** và tệp JSON vừa tạo. Máy chủ kiểm tra phiên bản đặc trưng, cấu trúc cây, các mẫu đối chiếu kết quả Python–Java, mã thiết bị và báo cáo tách nhóm trước khi lưu. Dự đoán cần khoảng hai giây thu mẫu cộng thời gian truyền/cập nhật trang. **Dừng thu** hủy phần chưa gửi/xác nhận của lần thu trên ESP32; các đoạn máy chủ đã nhận vẫn giữ để xem/gắn nhãn. **Gỡ mô hình AI** đưa hệ thống về trạng thái chưa huấn luyện; dữ liệu đã thu vẫn giữ lại.

## Giới hạn vận hành

- Kết quả phân loại không xác định mức hư hại hay bảo đảm kiện an toàn. Cảm biến có thể chạm giới hạn trong va đập mạnh.
- Chuỗi mẫu của lần thu chỉ có hàng đợi RAM giới hạn trên ESP32; thử gửi lại và chờ xác nhận database. Mất nguồn, mất mạng lâu hoặc đầy hàng đợi có thể mất đoạn. Dashboard báo số nhận và trạng thái thiếu; không âm thầm coi là thu đủ.
- Cơ chế NVS giữ sự kiện cảnh báo hiện có vẫn tách biệt với dữ liệu AI. LCD1602 dùng tiếng Việt không dấu vì không hỗ trợ Unicode; dashboard và trang cấu hình dùng tiếng Việt có dấu. Chỉ mẫu của lần thu được lưu lâu dài; đoạn trực tiếp dùng để phân tích không liên tục ghi mẫu thô vào database.
- Một mô hình dùng cho một thiết bị hiện tại. Đổi cảm biến, kiện, cách gắn hoặc điều kiện vận chuyển cần thu dữ liệu và đánh giá lại.
- Các tệp mô hình/dữ liệu cá nhân nên đặt tên `*.local.json` hoặc nằm trong thư mục `*.local` để Git bỏ qua.

