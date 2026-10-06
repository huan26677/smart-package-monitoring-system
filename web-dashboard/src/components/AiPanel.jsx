import {useEffect,useState} from "react";
import {CartesianGrid,Line,LineChart,ResponsiveContainer,Tooltip,XAxis,YAxis} from "recharts";
import {aiLabels,labelVi} from "../services/labels";
import {getAiStatus,startAiCapture,stopAiCapture,getAiWindows,getAiWindow,labelAiWindow,
  deleteAiCapture,exportAiDataset,importAiModel,removeAiModel} from "../services/api";

const percent=value=>`${(Number(value)*100).toLocaleString("vi-VN",{maximumFractionDigits:1})}%`;
const number=(value,digits=2)=>Number(value||0).toLocaleString("vi-VN",{maximumFractionDigits:digits});
const time=value=>new Date(value).toLocaleString("vi-VN");
const LIVE_STATUS={NO_MODEL:"Chưa huấn luyện",READY:"Đang phân tích",INVALID_DATA:"Chưa kết luận: đoạn đo không hợp lệ",OTHER_DEVICE:"Mô hình thuộc thiết bị khác"};
function Waveform({samples}) {
  const data=samples.map(r=>({ms:r[0]-samples[0][0],ax:r[1],ay:r[2],az:r[3],gx:r[4],gy:r[5],gz:r[6]}));
  return <div className="ai-waveforms">{[["Gia tốc ba trục (g)",["ax","ay","az"]],["Tốc độ góc ba trục (°/giây)",["gx","gy","gz"]]].map(([title,axes])=>
    <div key={title}><h4>{title}</h4><ResponsiveContainer width="100%" height={200}><LineChart data={data}>
      <CartesianGrid strokeDasharray="3 3"/><XAxis dataKey="ms" tickFormatter={v=>`${v} ms`}/><YAxis width={48}/>
      <Tooltip labelFormatter={v=>`${v} ms`} formatter={(v,name)=>[number(v,3),name]}/>
      {axes.map((axis,i)=><Line key={axis} dataKey={axis} name={["Trục X","Trục Y","Trục Z"][i]} stroke={["#2563eb","#d97706","#16a34a"][i]} dot={false} isAnimationActive={false}/>)}</LineChart></ResponsiveContainer></div>)}</div>;
}
function Evaluation({model}) {
  const e=model.evaluation;
  const classes=[...model.classes,"UNCERTAIN"];
  return <div className="ai-evaluation">
    <p>Đánh giá trên {e.testCount} đoạn thuộc {e.testGroups.length} nhóm buổi thử riêng; huấn luyện trên {e.trainCount} đoạn. Nhóm giữa hai tập không trùng nhau.</p>
    <div className="table-wrapper"><table><thead><tr><th>Chỉ số</th><th>AI</th><th>Quy tắc hiện tại</th></tr></thead><tbody>
      {[["Tỷ lệ đúng","accuracy"],["F1 trung bình","macroF1"],["Tỷ lệ có kết luận","coverage"]].map(([title,key])=><tr key={key}><td>{title}</td><td>{percent(e.ai[key])}</td><td>{percent(e.baseline[key])}</td></tr>)}</tbody></table></div>
    <div className="table-wrapper"><table><thead><tr><th>Loại quan sát</th><th>Độ chính xác</th><th>Tỷ lệ phát hiện</th><th>F1</th><th>Số mẫu kiểm tra</th></tr></thead><tbody>
      {model.classes.map(label=>{const m=e.ai.perClass[label];return <tr key={label}><td>{labelVi(label)}</td><td>{percent(m.precision)}</td><td>{percent(m.recall)}</td><td>{percent(m.f1)}</td><td>{m.support}</td></tr>;})}</tbody></table></div>
    <h4>Bảng nhầm lẫn của AI</h4><p className="meta">Hàng là nhãn quan sát; cột là dự đoán. “Chưa chắc chắn” được tính vào các trường hợp không phát hiện đúng.</p>
    <div className="table-wrapper"><table><thead><tr><th>Quan sát / Dự đoán</th>{classes.map(c=><th key={c}>{labelVi(c)}</th>)}</tr></thead><tbody>
      {model.classes.map((label,i)=><tr key={label}><th>{labelVi(label)}</th>{e.ai.confusion[i].map((n,j)=><td key={j}>{n}</td>)}</tr>)}</tbody></table></div>
    {e.ai.macroF1<=e.baseline.macroF1 && <p className="ai-warning">Trong tập kiểm tra này, AI chưa vượt quy tắc hiện tại về F1 trung bình. Cần xem lại dữ liệu và thử ở buổi mới.</p>}
  </div>;
}
export default function AiPanel({deviceId}) {
  const [status,setStatus]=useState(null),[captureId,setCaptureId]=useState(""),[windows,setWindows]=useState(null);
  const [page,setPage]=useState(0),[preview,setPreview]=useState(null),[reload,setReload]=useState(0);
  const [name,setName]=useState(""),[groupName,setGroupName]=useState(""),[duration,setDuration]=useState(30);
  const [error,setError]=useState(""),[busy,setBusy]=useState(false),[notice,setNotice]=useState("");
  const [now,setNow]=useState(()=>Date.now());
  useEffect(()=>{
    let active=true,inFlight=false;
    async function refresh(){if(inFlight)return;inFlight=true;setNow(Date.now());try {const data=await getAiStatus(deviceId);if(active){setStatus(data);setCaptureId(id=>id||data.captures[0]?.id||"");}}catch(e){if(active)setError(e.message);}finally{inFlight=false;}}
    refresh();const timer=setInterval(refresh,3000);return()=>{active=false;clearInterval(timer);};
  },[deviceId,reload]);
  useEffect(()=>{
    if(!captureId)return;
    let active=true,inFlight=false;
    async function refresh(){if(inFlight)return;inFlight=true;try{const data=await getAiWindows(deviceId,captureId,page);if(active)setWindows(data);}catch(e){if(active)setError(e.message);}finally{inFlight=false;}}
    refresh();const timer=setInterval(refresh,3000);return()=>{active=false;clearInterval(timer);};
  },[deviceId,captureId,page,reload]);
  async function action(fn,message="") {setBusy(true);setError("");setNotice("");try{await fn();setReload(v=>v+1);setNotice(message);}catch(e){setError(e.message);}finally{setBusy(false);}}
  const current=status?.captures.find(c=>c.id===captureId),live=status?.latest,model=status?.model;
  const liveFresh=live && now-new Date(live.receivedAt).getTime()<10000;
  const activeCapture=status?.captures.some(c=>c.status==="COLLECTING");
  const enough=status?.counts.filter(c=>aiLabels.includes(c.label)).every(c=>c.count>=30&&c.groups>=3);
  async function start(event) {event.preventDefault();await action(async()=>{const c=await startAiCapture(deviceId,{name,groupName,durationSeconds:Number(duration)});setCaptureId(c.id);setWindows(null);setPreview(null);setPage(0);},"Đã gửi lệnh thu. Quan sát thao tác và gắn nhãn từng đoạn sau khi nhận.");}
  return <section className="card ai-panel" aria-labelledby="ai-title"><h2 id="ai-title">AI phân biệt va đập</h2>
    <p className="meta">AI phân tích song song, không hủy cảnh báo theo ngưỡng. Chỉ ba nhóm: bình thường, rung lắc và va đập; chưa kết luận mức hư hại của hàng.</p>
    {error&&<div className="error" role="alert">{error}</div>}{notice&&<p role="status">{notice}</p>}
    <div className="ai-live"><strong>{!liveFresh?"Chưa nhận đoạn cảm biến mới":LIVE_STATUS[live.status]||"Chưa xác định"}</strong>
      <div className="ai-live-grid"><div><span>Kết quả AI</span><strong>{liveFresh&&live.label?labelVi(live.label):"Chưa có kết luận"}</strong></div>
        <div><span>Điểm dự đoán</span><strong>{liveFresh&&live.score!=null?percent(live.score):"—"}</strong></div>
        <div><span>Kết quả theo ngưỡng</span><strong>{liveFresh?labelVi(live.ruleLabel):"—"}</strong></div>
        <div><span>Đỉnh gia tốc của đoạn</span><strong>{liveFresh?`${number(live.features[0])} g`:"—"}</strong></div></div>
      {live&&<p className="meta">Nhận lúc {time(live.receivedAt)} · Nhịp đo trung bình {number(1000/live.features[14],1)} Hz. Điểm dự đoán không phải độ chắc chắn đã được hiệu chuẩn.</p>}
    </div>
    <h3>1. Thu dữ liệu thật</h3><p>Gắn cảm biến cố định vào kiện mẫu. Nhập cùng một nhóm buổi thử cho các lần thu liên quan trong cùng điều kiện; chỉ đổi nhóm khi thực sự bắt đầu một buổi/lần thử độc lập.</p>
    <form className="ai-capture-form" onSubmit={start}>
      <label>Tên lần thu<input value={name} onChange={e=>setName(e.target.value)} maxLength={120} placeholder="Ví dụ: Mang kiện nhẹ nhàng" required/></label>
      <label>Nhóm buổi thử<input value={groupName} onChange={e=>setGroupName(e.target.value)} maxLength={120} placeholder="Ví dụ: Buổi 1 – kiện mẫu A" required/></label>
      <label>Thời gian thu<select aria-label="Thời gian thu" value={duration} onChange={e=>setDuration(e.target.value)}>{[10,30,60,120].map(v=><option key={v} value={v}>{v} giây · {v/2} đoạn</option>)}</select></label>
      <button type="submit" disabled={busy||!liveFresh||activeCapture}>Bắt đầu thu</button>
    </form>
    {!liveFresh&&<p className="ai-warning">ESP32 cần firmware hỗ trợ AI và kết nối MQTT để gửi các đoạn đo mới.</p>}
    <div className="ai-capture-toolbar"><label>Lần thu đã lưu<select aria-label="Lần thu đã lưu" value={captureId} onChange={e=>{setCaptureId(e.target.value);setPage(0);setPreview(null);setWindows(null);}}>
      <option value="">Chọn lần thu</option>{status?.captures.map(c=><option key={c.id} value={c.id}>{c.name} · {c.receivedWindows}/{c.expectedWindows} đoạn · {labelVi(c.status)}</option>)}</select></label>
      {current?.status==="COLLECTING"&&<button disabled={busy} onClick={()=>action(()=>stopAiCapture(deviceId,captureId),"Đã gửi lệnh dừng thu.")}>Dừng thu</button>}
      {current&&current.status!=="COLLECTING"&&<button disabled={busy} onClick={()=>{if(window.confirm("Xóa lần thu này và toàn bộ nhãn/đoạn cảm biến của nó?"))action(async()=>{await deleteAiCapture(deviceId,captureId);setCaptureId("");setWindows(null);setPreview(null);},"Đã xóa lần thu.");}}>Xóa lần thu</button>}
    </div>
    {current&&<p>Đã nhận <strong>{current.receivedWindows}/{current.expectedWindows}</strong> đoạn · {labelVi(current.status)} · Nhóm: {current.groupName}. Các đoạn còn trong RAM của ESP32 có thể mất khi tắt nguồn; không coi lần thu thiếu đoạn là đầy đủ.</p>}
    <h3>2. Gắn nhãn theo thao tác quan sát</h3><p>Chọn nhãn cho từng đoạn sau khi xem tín hiệu. Không lấy kết quả theo ngưỡng hoặc dự đoán AI làm đáp án. Đoạn không rõ thao tác nên để “Chưa gắn nhãn”.</p>
    {windows?.content.length>0?<><div className="table-wrapper"><table><thead><tr><th>Đoạn</th><th>Giờ nhận</th><th>Đỉnh gia tốc</th><th>Chất lượng đo</th><th>Nhãn quan sát</th><th>Tín hiệu</th></tr></thead><tbody>
      {windows.content.map(w=><tr key={w.id}><td>{w.sequence}</td><td>{time(w.receivedAt)}</td><td>{number(w.features[0])} g</td>
        <td>{w.saturated?"Chạm giới hạn đo":!w.timingValid?"Nhịp đo không đều":"Đủ điều kiện"}</td>
        <td><select aria-label={`Nhãn đoạn ${w.sequence}`} value={w.label} disabled={busy} onChange={e=>action(()=>labelAiWindow(deviceId,w.id,e.target.value),"Đã lưu nhãn quan sát.")}>
          {["UNLABELED",...aiLabels].map(v=><option key={v} value={v}>{labelVi(v)}</option>)}</select></td>
        <td><button disabled={busy} onClick={()=>action(async()=>setPreview(await getAiWindow(deviceId,w.id)))}>Xem đoạn</button></td></tr>)}</tbody></table></div>
      <div className="pagination"><button disabled={page===0||busy} onClick={()=>setPage(v=>v-1)}>Trang trước</button><span>Trang {page+1}/{windows.totalPages}</span><button disabled={page+1>=windows.totalPages||busy} onClick={()=>setPage(v=>v+1)}>Trang sau</button></div></>:<p className="empty">Chưa có đoạn cảm biến trong lần thu đã chọn.</p>}
    {preview&&<div className="ai-preview"><h4>Tín hiệu đoạn {preview.sequence} · {labelVi(preview.label)}</h4><Waveform samples={preview.samples}/></div>}
    <h3>3. Huấn luyện và đánh giá</h3><div className="ai-counts">{status?.counts.map(c=><div key={c.label}><strong>{labelVi(c.label)}</strong><span>{c.count} đoạn đủ điều kiện · {c.groups} nhóm buổi thử</span></div>)}</div>
    <p>{enough?"Đã đạt số lượng tối thiểu để thử huấn luyện; vẫn cần kiểm tra chất lượng nhãn và tính độc lập của các buổi thử.":"Cần ít nhất 30 đoạn đã gắn nhãn và 3 nhóm buổi thử độc lập cho mỗi loại. Đoạn chạm giới hạn hoặc nhịp đo không hợp lệ không được xuất để huấn luyện."}</p>
    <div className="ai-model-actions"><button disabled={busy} onClick={()=>action(()=>exportAiDataset(deviceId),"Đã xuất bộ dữ liệu để huấn luyện.")}>Xuất dữ liệu huấn luyện</button>
      <label className="ai-file-label">Nạp mô hình đã huấn luyện<span className="ai-file-button">Chọn tệp JSON<input className="ai-file-input" aria-label="Chọn tệp mô hình JSON" type="file" accept=".json,application/json" disabled={busy} onChange={e=>{const file=e.target.files[0];if(file)action(()=>importAiModel(deviceId,file),"Đã kiểm tra và nạp mô hình AI.");e.target.value="";}}/></span></label>
      {model?.available&&<button disabled={busy} onClick={()=>{if(window.confirm("Gỡ mô hình AI hiện tại? Dữ liệu và cảnh báo theo ngưỡng vẫn tiếp tục hoạt động."))action(()=>removeAiModel(deviceId),"Đã gỡ mô hình AI.");}}>Gỡ mô hình AI</button>}
    </div>
    <p className="meta">Huấn luyện trên máy tính bằng ai-training/train.py với tệp vừa xuất. Mô hình nạp vào cần có báo cáo đánh giá trên các nhóm buổi thử riêng, đủ mẫu thật và đúng thiết bị.</p>
    {model?.available?<><p>Phiên bản: {model.version} · Huấn luyện lúc {time(model.trainedAt)} · Ngưỡng điểm: {percent(model.scoreThreshold)}</p><Evaluation model={model}/></>:<p className="ai-warning">Chưa huấn luyện mô hình AI. Hãy thu và gắn nhãn dữ liệu thật trước.</p>}
  </section>;
}
