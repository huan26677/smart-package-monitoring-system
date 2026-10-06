import { useState } from "react";

function readAcknowledgedCounter(key) {
  try {
    const value = localStorage.getItem(key);
    const counter = value === null ? null : Number(value);
    return Number.isSafeInteger(counter) && counter >= 0 ? counter : null;
  } catch {
    return null;
  }
}

export default function EventStorageNotice({ deviceId, telemetry, online }) {
  const key = `acknowledged-storage-loss:${deviceId}`;
  const rejected = telemetry.rejectedEvents;
  const pending = telemetry.pendingEvents;
  const hasCounter = Number.isSafeInteger(rejected) && rejected >= 0;
  const [acknowledged, setAcknowledged] = useState(() => readAcknowledgedCounter(key));
  const [baseline, setBaseline] = useState(() => {
    if (!hasCounter) return 0;
    return acknowledged !== null && acknowledged <= rejected ? acknowledged : rejected;
  });
  const newLosses = hasCounter ? Math.max(0, rejected - baseline) : 0;
  // A different counter must not be hidden by an earlier acknowledgement.
  const alreadySeen = hasCounter && rejected === acknowledged;

  function acknowledge() {
    try { localStorage.setItem(key, String(rejected)); } catch { /* Keep working when storage is disabled. */ }
    setAcknowledged(rejected);
    setBaseline(rejected);
  }

  const syncText = online && pending === 0
    ? "Hiện không có sự kiện chờ đồng bộ."
    : online && pending > 0
      ? `Hiện có ${pending} sự kiện chờ đồng bộ.`
      : "Thiết bị mất kết nối; chưa xác định được tình trạng đồng bộ hiện tại.";

  return <>
    {pending > 0 && <section className="alert-banner alert-warning" role="status">
      <div className="alert-title">Có {pending} sự kiện chờ đồng bộ</div>
      <p>Thiết bị gửi lại sự kiện khi có kết nối, cho đến khi máy chủ xác nhận đã lưu dữ liệu.</p>
    </section>}
    {hasCounter && rejected > 0 && (!alreadySeen
      ? <section className={`alert-banner ${newLosses > 0 ? "alert-danger" : "alert-warning"}`}
          role={newLosses > 0 ? "alert" : "status"}>
        <div className="alert-title">{newLosses > 0
          ? "Có sự kiện mới không được lưu"
          : "Thiết bị từng không lưu được sự kiện"}</div>
        {newLosses > 0 && <p>{newLosses} sự kiện không được lưu thêm kể từ khi mở trang hoặc xác nhận lần gần nhất. Kiểm tra kết nối và bộ nhớ thiết bị.</p>}
        <p>Tổng số sự kiện không được lưu đã ghi nhận: <strong>{rejected}</strong>. Đây là số tích lũy, không phải số sự kiện đang chờ hay dung lượng bộ nhớ đang sử dụng.</p>
        <p>{syncText}</p>
        <button type="button" onClick={acknowledge}>Đã xem thông báo lưu sự kiện</button>
      </section>
      : <details className="storage-history">
        <summary>Lịch sử lưu sự kiện: {rejected} sự kiện không được lưu · Đã xem</summary>
        <p>Số tích lũy do thiết bị báo; xác nhận thông báo chỉ thu gọn nội dung. {syncText}</p>
      </details>)}
  </>;
}
