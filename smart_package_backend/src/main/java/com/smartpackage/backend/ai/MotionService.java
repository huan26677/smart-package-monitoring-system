package com.smartpackage.backend.ai;

import java.time.Instant;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import org.springframework.data.domain.PageRequest;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;
import tools.jackson.databind.json.JsonMapper;
import com.smartpackage.backend.repository.DeviceRepository;

@Service
public class MotionService {
    private final AiCaptureRepository captures;
    private final AiWindowRepository windows;
    private final AiModelService models;
    private final DeviceRepository devices;
    private final JsonMapper mapper;
    private final Map<String,Live> latest=new ConcurrentHashMap<>();
    public record Live(Instant receivedAt,long timestamp,String ruleLabel,String status,String label,String candidate,
            Double score,String modelVersion,List<Double> features,boolean timingValid,boolean saturated) {}
    public record CaptureView(String id,String name,String groupName,Instant createdAt,int expectedWindows,long receivedWindows,String status) {}
    public record WindowView(Long id,int sequence,String label,long timestamp,Instant receivedAt,String ruleLabel,
            List<Double> features,boolean timingValid,boolean saturated,Object samples) {}
    public MotionService(AiCaptureRepository captures,AiWindowRepository windows,AiModelService models,
            DeviceRepository devices,JsonMapper mapper) {this.captures=captures;this.windows=windows;this.models=models;this.devices=devices;this.mapper=mapper;}
    @Transactional
    public String accept(String topic,String json) {
        if(json.length()>30000) throw new IllegalArgumentException("Đoạn cảm biến vượt giới hạn.");
        MotionWindow data=mapper.readValue(json,MotionWindow.class);
        if(data.deviceId()==null || !data.deviceId().matches("[A-Za-z0-9_-]{1,64}")
                || !topic.equals("smart-package/"+data.deviceId()+"/motion-window") || data.schemaVersion()!=1
                || data.bootId()==null || !data.bootId().matches("[a-f0-9]{32}") || data.windowId()<1
                || data.timestamp()<0 || !ForestModel.LABELS.contains(data.ruleLabel()))
            throw new IllegalArgumentException("Thông tin đoạn cảm biến không hợp lệ.");
        var features=MotionFeatures.extract(data.samples());
        var model=models.current();
        String state=model==null?"NO_MODEL":"READY",label=null,candidate=null,version=model==null?null:model.version();Double score=null;
        if(model!=null && !model.deviceId().equals(data.deviceId())) state="OTHER_DEVICE";
        else if(!features.timingValid() || features.saturated()) state="INVALID_DATA";
        else if(model!=null) {var p=model.predict(features.values());label=p.label();candidate=p.candidate();score=p.score();}
        latest.put(data.deviceId(),new Live(Instant.now(),data.timestamp(),data.ruleLabel(),state,label,candidate,score,version,
                features.values(),features.timingValid(),features.saturated()));
        if(data.sessionId()==null || data.sessionId().isBlank()) return null;
        var capture=owned(data.deviceId(),data.sessionId());
        if(data.sessionWindow()<1 || data.sessionWindow()>capture.expectedWindows) throw new IllegalArgumentException("Số thứ tự đoạn ngoài buổi thu.");
        if(!windows.existsByCapture_IdAndSequence(capture.id,data.sessionWindow())) {
            if(Instant.now().isAfter(capture.deadline.plusSeconds(30))) throw new IllegalArgumentException("Buổi thu đã hết thời gian nhận dữ liệu.");
            windows.saveAndFlush(new AiWindowEntity(capture,data,mapper.writeValueAsString(data.samples()),
                    mapper.writeValueAsString(features.values()),features));
            if(windows.countByCapture_Id(capture.id)>=capture.expectedWindows) capture.stopped=true;
        }
        return mapper.writeValueAsString(Map.of("sessionId",capture.id,"sessionWindow",data.sessionWindow(),"status","SAVED"));
    }
    @Transactional
    public AiCapture create(String deviceId,String name,String groupName,int duration) {
        if(name==null || name.isBlank() || name.length()>120 || groupName==null || groupName.isBlank() || groupName.length()>120)
            throw new IllegalArgumentException("Nhập tên lần thu và nhóm buổi thử, tối đa 120 ký tự.");
        if(duration<10 || duration>120 || duration%2!=0) throw new IllegalArgumentException("Thời gian thu phải là số chẵn từ 10 đến 120 giây.");
        var device=devices.findForCapture(deviceId).orElseThrow(()->new ResponseStatusException(HttpStatus.NOT_FOUND,"Không tìm thấy thiết bị."));
        var live=latest.get(deviceId);
        if(device.getLastSeenAt().isBefore(Instant.now().minusSeconds(15)) || live==null || live.receivedAt().isBefore(Instant.now().minusSeconds(10)))
            throw new ResponseStatusException(HttpStatus.CONFLICT,"ESP32 chưa gửi đoạn cảm biến mới. Kiểm tra kết nối và nạp firmware hỗ trợ AI.");
        if(captures.existsByDeviceIdAndStoppedFalseAndDeadlineAfter(deviceId,Instant.now()))
            throw new ResponseStatusException(HttpStatus.CONFLICT,"Thiết bị đang có một lần thu chưa kết thúc. Dừng lần thu trước hoặc chờ hết thời gian.");
        return captures.saveAndFlush(new AiCapture(deviceId,name.trim(),groupName.trim(),duration));
    }
    @Transactional
    public void stop(String deviceId,String id) {var c=owned(deviceId,id);c.stopped=true;}
    @Transactional(readOnly=true)
    public Object status(String deviceId) {
        List<Map<String,Object>> counts=new ArrayList<>();
        var rows=windows.labelCounts(deviceId);
        for(String label:List.of("NORMAL","VIBRATION","IMPACT","UNLABELED")) {
            var row=rows.stream().filter(r->label.equals(r[0])).findFirst().orElse(new Object[]{label,0L,0L});
            counts.add(Map.of("label",label,"count",row[1],"groups",row[2]));
        }
        var result=new LinkedHashMap<String,Object>();result.put("model",models.info());result.put("latest",latest.get(deviceId));
        result.put("counts",counts);result.put("featureNames",MotionFeatures.NAMES);
        result.put("captures",captures.findTop50ByDeviceIdOrderByCreatedAtDesc(deviceId).stream().map(this::captureView).toList());
        return result;
    }
    public CaptureView captureView(AiCapture c) {
        long count=windows.countByCapture_Id(c.id);
        String state=count>=c.expectedWindows?"COMPLETE":c.stopped?"STOPPED":Instant.now().isAfter(c.deadline)?"INCOMPLETE":"COLLECTING";
        return new CaptureView(c.id,c.name,c.groupName,c.createdAt,c.expectedWindows,count,state);
    }
    @Transactional(readOnly=true)
    public Object list(String deviceId,String id,int page) {
        owned(deviceId,id);if(page<0) throw new IllegalArgumentException("Trang không hợp lệ.");
        var result=windows.findByCapture_IdOrderBySequenceAsc(id,PageRequest.of(page,15));
        return Map.of("content",result.stream().map(e->view(e,false)).toList(),"totalPages",result.getTotalPages(),"page",page,"totalElements",result.getTotalElements());
    }
    @Transactional(readOnly=true)
    public WindowView detail(String deviceId,long id) {return view(ownedWindow(deviceId,id),true);}
    @Transactional
    public void label(String deviceId,long id,String label) {
        if(label==null || !(ForestModel.LABELS.contains(label)||label.equals("UNLABELED"))) throw new IllegalArgumentException("Nhãn không hợp lệ.");
        ownedWindow(deviceId,id).label=label;
    }
    @Transactional
    public void delete(String deviceId,String id) {
        var c=owned(deviceId,id);
        if(!c.stopped && Instant.now().isBefore(c.deadline)) throw new IllegalArgumentException("Dừng thu trước khi xóa dữ liệu.");
        windows.deleteByCapture_Id(id);captures.delete(c);
    }
    @Transactional(readOnly=true)
    public Object dataset(String deviceId) {
        var rows=windows.dataset(deviceId,PageRequest.of(0,10001));
        if(rows.size()>10000) throw new IllegalArgumentException("Bộ dữ liệu vượt 10.000 đoạn. Xóa các lần thu không dùng trước khi xuất.");
        List<Map<String,Object>> samples=new ArrayList<>();
        for(var e:rows) samples.add(Map.of("id",e.id,"group",e.capture.groupName,"captureId",e.capture.id,"label",e.label,
                "features",mapper.readValue(e.featuresJson,List.class),"ruleLabel",e.ruleLabel));
        return Map.of("schemaVersion",1,"source","real-device","deviceId",deviceId,"exportedAt",Instant.now(),"featureNames",MotionFeatures.NAMES,"samples",samples);
    }
    private WindowView view(AiWindowEntity e,boolean raw) {
        @SuppressWarnings("unchecked") List<Double> features=mapper.readValue(e.featuresJson,List.class);
        // JSON numeric types are kept for serialization; callers do not do arithmetic on this list.
        return new WindowView(e.id,e.sequence,e.label,e.timestamp,e.receivedAt,e.ruleLabel,features,e.timingValid,e.saturated,
                raw?mapper.readValue(e.samplesJson,List.class):null);
    }
    private AiCapture owned(String deviceId,String id) {return captures.findById(id).filter(c->c.deviceId.equals(deviceId))
        .orElseThrow(()->new ResponseStatusException(HttpStatus.NOT_FOUND,"Không tìm thấy lần thu dữ liệu."));}
    private AiWindowEntity ownedWindow(String deviceId,long id) {return windows.findById(id).filter(w->w.capture.getDeviceId().equals(deviceId))
        .orElseThrow(()->new ResponseStatusException(HttpStatus.NOT_FOUND,"Không tìm thấy đoạn cảm biến."));}
}
