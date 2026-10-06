import {labelVi} from "../services/labels";
import { useEffect, useState } from "react";
import EventTable from "./EventTable";
import { getEventHistory, getEventSummary, exportEvents } from "../services/api";

export default function EventHistoryPanel({deviceId}) {
  const [filters, setFilters] = useState({type:"",level:"",from:"",to:""});
  const [page, setPage] = useState(0);
  const [result, setResult] = useState(null);
  const [summary, setSummary] = useState(null);
  const [error, setError] = useState("");
  const [exporting, setExporting] = useState(false);
  function query() {
    return {...filters, from:filters.from ? new Date(filters.from).toISOString() : "",
      to:filters.to ? new Date(filters.to).toISOString() : ""};
  }
  useEffect(() => {
    let active = true;
    let inFlight = false;
    const params = {...filters, from:filters.from ? new Date(filters.from).toISOString() : "",
      to:filters.to ? new Date(filters.to).toISOString() : ""};
    async function refresh() {
      if(inFlight) return;
      inFlight = true;
      try {
        const [history, stats] = await Promise.all([
          getEventHistory(deviceId,{...params,page,size:20}), getEventSummary(deviceId,params)]);
        if(active) {setResult(history);setSummary(stats);setError("");}
      } catch(e) {if(active) {setResult(null);setSummary(null);setError(e.message);}}
      finally {inFlight = false;}
    }
    refresh(); const interval = setInterval(refresh,3000);
    return () => {active = false;clearInterval(interval);};
  },[deviceId,filters,page]);
  function filter(name,value) {setPage(0);setFilters(current=>({...current,[name]:value}));}
  async function download() {
    setExporting(true);
    try {await exportEvents(deviceId,query());setError("");}
    catch(e) {setError(e.message);}
    finally {setExporting(false);}
  }
  return <section className="card event-card">
    <h2>Lịch sử sự kiện kiện hàng</h2>
    <p className="meta">Thống kê toàn bộ sự kiện theo bộ lọc. Các mức gia tốc dùng ngưỡng thử nghiệm, chưa xác định mức hư hại của hàng.</p>
    <div className="event-filters">
      <label>Loại sự kiện<select aria-label="Loại sự kiện" value={filters.type} onChange={e=>filter("type",e.target.value)}>
        <option value="">Tất cả</option>{["IMPACT","DROP","FREE_FALL","TILT","FLIP","VIBRATION"].map(v=><option key={v} value={v}>{labelVi(v)}</option>)}</select></label>
      <label>Mức va đập<select aria-label="Mức va đập" value={filters.level} onChange={e=>filter("level",e.target.value)}>
        <option value="">Tất cả</option>{["NONE","LIGHT","MEDIUM","STRONG"].map(v=><option key={v} value={v}>{labelVi(v)}</option>)}</select></label>
      <label>Từ thời điểm<input type="datetime-local" value={filters.from} onChange={e=>filter("from",e.target.value)} /></label>
      <label>Đến thời điểm<input type="datetime-local" value={filters.to} onChange={e=>filter("to",e.target.value)} /></label>
      <button onClick={download} disabled={exporting || !result}>{exporting ? "Đang xuất…" : "Xuất CSV"}</button>
    </div>
    {error && <div className="error" role="alert">{error}</div>}
    {summary && <div className="event-summary">
      <span>Tổng sự kiện: <strong>{summary.totalEvents}</strong></span>
      <span>Va đập: <strong>{summary.impacts}</strong></span>
      <span>Rơi: <strong>{summary.drops}</strong></span>
      <span>Mức mạnh: <strong>{summary.strongEvents}</strong></span>
      <span>G lớn nhất ghi nhận: <strong>{summary.maxG.toFixed(2)} g</strong></span>
      <span>Chạm giới hạn đo: <strong>{summary.saturatedEvents}</strong></span>
    </div>}
    {result && <><EventTable events={result.content} /><div className="pagination">
      <button onClick={()=>setPage(p=>p-1)} disabled={page===0}>Trang trước</button>
      <span>Trang {result.totalPages ? page+1 : 0}/{result.totalPages} · {result.totalElements} sự kiện</span>
      <button onClick={()=>setPage(p=>p+1)} disabled={page+1>=result.totalPages}>Trang sau</button>
    </div></>}
    <p className="meta">Bộ lọc thời gian dùng giờ xảy ra; sự kiện chưa đồng bộ NTP chỉ xuất hiện khi không lọc thời gian. CSV gồm toàn bộ kết quả lọc, tối đa 10.000 sự kiện.</p>
  </section>;
}
