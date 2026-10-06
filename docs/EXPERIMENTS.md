# Thực nghiệm giám sát va đập kiện hàng

Đây là kế hoạch thực nghiệm, chưa phải kết quả đo. Chỉ điền số liệu đã quan sát; không dùng dữ liệu MQTT giả lập làm bằng chứng độ chính xác cảm biến.

## Chuẩn bị

1. Dùng một ESP32 `esp32-001`, ghi rõ model cảm biến thực tế, phiên bản firmware, ngày thử và cách gắn cảm biến vào kiện mẫu. Cố định cảm biến để chuyển động của nó đại diện cho chuyển động kiện hàng.
2. Ghi khối lượng, kích thước, vật liệu đệm, mặt tiếp xúc và hướng thả của kiện mẫu. Giữ các yếu tố này ổn định giữa các lần thử cùng nhóm.
3. Sau mỗi lần khởi động, đặt kiện đứng yên để lấy tư thế chuẩn. Đợi NTP, kiểm tra telemetry cập nhật và `pendingEvents=0`. Ghi lại `rejectedEvents` trước thử; đây là bộ đếm tích lũy, cần đối chiếu mức tăng sau từng lượt thay vì yêu cầu luôn bằng 0.
4. Ghi thời gian thực, nhãn quan sát và số lần thao tác bằng video hoặc người ghi độc lập. Xuất CSV từ dashboard sau từng nhóm. Đối chiếu bằng ID, giờ xảy ra và khoảng thời gian thao tác; một lần thả có thể tạo FREE_FALL rồi DROP, không tính chúng thành hai lần thả.

## Kịch bản

| Nhóm | Thao tác | Dữ liệu cần đối chiếu |
| --- | --- | --- |
| Bình thường | Đứng yên 5 phút; mang kiện nhẹ nhàng | Số cảnh báo IMPACT/DROP sai trên mỗi phút |
| Rung | Rung kiện có kiểm soát, ghi thời gian từng đợt | VIBRATION, RMS, số IMPACT/DROP bị báo nhầm |
| Nghiêng/lật | Thay đổi tư thế so với chuẩn ban đầu | TILT/FLIP, góc, thời gian phát hiện |
| Va đập | Tạo các đợt va đập riêng biệt với cùng cách tác động | Số đợt phát hiện, đỉnh g, mức, thời lượng, cờ giới hạn đo |
| Thả rơi | Thả kiện mẫu ở các độ cao chọn trước, giữ cùng hướng và mặt tiếp xúc | FREE_FALL/DROP, đỉnh ghi nhận, số đợt bỏ sót |
| Mất backend | Dừng backend, thực hiện thao tác, bật lại backend | Pending không được coi là đã lưu; sau phục hồi mỗi ID xuất hiện đúng một lần |
| Mất Wi-Fi | Ngắt Wi-Fi trên 30 giây, thực hiện thao tác, bật lại | Không cần setup lại, tự gửi sự kiện tồn đọng |
| Khởi động lại | Chờ sự kiện ghi flash rồi khởi động lại thiết bị | Bản ghi NVS chưa xác nhận được gửi lại; không trùng ID |

Đề xuất tối thiểu 30 lần mỗi nhóm va đập/thả và nhiều nhóm thao tác bình thường để tính độ lặp lại. Chọn độ cao dựa trên kiện mẫu và điều kiện của phòng thử, ghi rõ độ cao thực tế. Tách một nhóm chỉnh ngưỡng và một nhóm đánh giá cuối; không đánh giá chỉ trên dữ liệu đã dùng để chỉnh ngưỡng.

## Ghi và tính kết quả

Dùng [experiment-template.csv](experiment-template.csv), bổ sung một dòng cho mỗi thao tác. Các cột `detected_event_ids` ghi những ID liên quan; `tp/fp/fn` được quyết định sau khi đối chiếu với nhãn quan sát. Với bài toán phát hiện va đập/thả, quy định trước khoảng ghép thời gian và chính sách “một thao tác, một lần phát hiện”.

- TP: thao tác thật được phát hiện đúng; FN: thao tác thật bị bỏ sót; FP: hệ thống báo va đập/thả khi không có thao tác đó.
- Precision = TP / (TP + FP); Recall = TP / (TP + FN); F1 = 2 × Precision × Recall / (Precision + Recall). Khi mẫu số bằng 0, ghi “chưa đủ dữ liệu”.
- Báo số cảnh báo sai/phút ở nhóm bình thường, trung bình và độ lệch chuẩn đỉnh g của từng nhóm, số lần chạm giới hạn đo, số bản ghi bị từ chối, thời gian đồng bộ lại sau mất kết nối.
- Độ trễ phát hiện và độ trễ mạng phải được báo riêng: firmware chờ khoảng yên 80 ms để chốt đỉnh, trong khi dashboard polling 3 giây và backend có giờ nhận riêng.

Các ngưỡng hiện tại: LIGHT từ 2,5 g, MEDIUM từ 4 g, STRONG từ 7 g. Đây là mức thuật toán theo gia tốc ghi nhận, chưa chứng minh tương ứng với mức hư hại của sản phẩm. Quan sát hư hại của hàng là một nhãn riêng, phụ thuộc loại hàng và đóng gói.

## Giới hạn cần nêu trong báo cáo

Vòng đọc danh định 100 Hz có thể bỏ qua xung ngắn giữa hai mẫu. Không gọi `peak_g` là đỉnh gia tốc vật lý chính xác nếu chưa đối chiếu với thiết bị tham chiếu có tốc độ đo phù hợp. Cảm biến ±16 g trên mỗi trục có thể bị giới hạn; cờ gần giới hạn chỉ báo mẫu đã quan sát, không chứng minh không có xung vượt giới hạn giữa các mẫu. Dữ liệu chạm giới hạn không dùng để kết luận đỉnh thật hoặc quan hệ độ cao–gia tốc định lượng.

NVS chỉ giữ tối đa 128 sự kiện, hàng đợi ghi RAM 32 bản ghi chưa bền vững khi mất nguồn. Không tuyên bố chống mất dữ liệu vô hạn. Thử khởi động lại chỉ xác nhận bản ghi đã ghi NVS, còn thử mất nguồn tức thời cần một bài đo riêng. Kiểm thử mô phỏng xác nhận logic phần mềm; kiểm thử vật lý xác nhận khả năng đo trong điều kiện đã mô tả.
