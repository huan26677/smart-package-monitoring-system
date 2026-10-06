import {labelVi} from "../services/labels";
import { useState } from "react";
import { formatOccurredAt } from "../services/eventFormat";

export default function EventNotice({deviceId, events}) {
  const key = `acknowledged-event:${deviceId}`;
  const [acknowledged, setAcknowledged] = useState(() => {
    try { return Number(localStorage.getItem(key) || 0); } catch { return 0; }
  });
  const pending = events.filter(e => ["IMPACT", "DROP"].includes(e.type) && e.id > acknowledged);
  if (!pending.length) return null;
  const event = pending[0];
  function acknowledge() {
    const id = Math.max(...pending.map(e => e.id));
    try { localStorage.setItem(key, String(id)); } catch { /* Browsers may disable storage. */ }
    setAcknowledged(id);
  }
  return <section className="alert-banner alert-danger" role="alert">
    <div className="alert-title">{event.type === "DROP" ? "Rơi kiện hàng đã ghi nhận" : "Va đập kiện hàng đã ghi nhận"}</div>
    <p>Đỉnh {event.gForce.toFixed(2)} g · Mức {labelVi(event.level)} · {formatOccurredAt(event)}</p>
    <p>{pending.length} sự kiện va đập/rơi chưa xem trong dữ liệu gần nhất. Cảnh báo được giữ đến khi bạn xác nhận.</p>
    <p>Đây là sự kiện trong lịch sử. Trạng thái hiện tại của kiện hàng hiển thị ở phía trên.</p>
    <button onClick={acknowledge}>Đã xem cảnh báo</button>
  </section>;
}
