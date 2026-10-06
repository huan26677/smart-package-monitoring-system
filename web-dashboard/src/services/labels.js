export const labels = {NORMAL:"Bình thường",VIBRATION:"Rung lắc",IMPACT:"Va đập",DROP:"Rơi va đập",FREE_FALL:"Rơi tự do",
  TILT:"Nghiêng",FLIP:"Lật",CALIBRATING:"Đang lấy tư thế chuẩn",UNKNOWN:"Chưa xác định",UNCERTAIN:"Chưa chắc chắn",
  UNLABELED:"Chưa gắn nhãn",NONE:"Không",LIGHT:"Nhẹ",MEDIUM:"Trung bình",STRONG:"Mạnh",
  PENDING:"Đang xác định",NO_ANCHOR:"Chưa xác định vị trí",ANCHOR_MATCHED:"Đã nhận diện vị trí",COLLECTING:"Đang thu / chờ đồng bộ",
  COMPLETE:"Đã nhận đủ",STOPPED:"Đã dừng",INCOMPLETE:"Thiếu đoạn đo"};
export function labelVi(value) {return labels[value] || value || "Chưa có";}
export const aiLabels=["NORMAL","VIBRATION","IMPACT"];
