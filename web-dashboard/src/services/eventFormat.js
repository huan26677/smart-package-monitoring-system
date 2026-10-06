export function occurredDate(event) {
  return Number(event.timestamp) > 0 ? new Date(Number(event.timestamp) * 1000) : null;
}
export function formatOccurredAt(event) {
  const date = occurredDate(event);
  return date ? date.toLocaleString("vi-VN") : "Chưa đồng bộ giờ (NTP)";
}
export function formatReceivedAt(event) {
  return event.receivedAt ? new Date(event.receivedAt).toLocaleString("vi-VN") : "--";
}
