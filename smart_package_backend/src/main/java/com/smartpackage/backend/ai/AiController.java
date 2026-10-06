package com.smartpackage.backend.ai;
import java.util.Map;
import org.springframework.http.*;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import com.smartpackage.backend.mqtt.MqttSubscriber;
import tools.jackson.databind.json.JsonMapper;
@RestController @RequestMapping("/api/devices/{deviceId}/ai")
public class AiController {
    private final MotionService motion;
    private final AiModelService models;
    private final MqttSubscriber mqtt;
    private final JsonMapper mapper;
    public record Start(String name,String groupName,int durationSeconds) {}
    public record Label(String label) {}
    public AiController(MotionService motion,AiModelService models,MqttSubscriber mqtt,JsonMapper mapper) {this.motion=motion;this.models=models;this.mqtt=mqtt;this.mapper=mapper;}
    @GetMapping public Object status(@PathVariable String deviceId) {return motion.status(deviceId);}
    @PostMapping("/captures") public Object start(@PathVariable String deviceId,@RequestBody Start input) {
        if(!mqtt.isReady()) throw new org.springframework.web.server.ResponseStatusException(HttpStatus.SERVICE_UNAVAILABLE,"Máy chủ chưa kết nối MQTT.");
        var capture=motion.create(deviceId,input.name(),input.groupName(),input.durationSeconds());
        try {mqtt.publishMotionControl(deviceId,mapper.writeValueAsString(Map.of("action","START","sessionId",capture.id,"windows",capture.expectedWindows)));}
        catch(Exception e) {motion.stop(deviceId,capture.id);throw new org.springframework.web.server.ResponseStatusException(HttpStatus.SERVICE_UNAVAILABLE,"Không gửi được lệnh thu tới ESP32. Hãy thử lại.");}
        return motion.captureView(capture);
    }
    @PostMapping("/captures/{id}/stop") public void stop(@PathVariable String deviceId,@PathVariable String id) {
        try {mqtt.publishMotionControl(deviceId,mapper.writeValueAsString(Map.of("action","STOP","sessionId",id)));}
        catch(Exception e) {throw new org.springframework.web.server.ResponseStatusException(HttpStatus.SERVICE_UNAVAILABLE,"Chưa gửi được lệnh dừng. Thiết bị vẫn tự dừng sau số đoạn đã chọn.");}
        motion.stop(deviceId,id);
    }
    @GetMapping("/captures/{id}/windows") public Object windows(@PathVariable String deviceId,@PathVariable String id,@RequestParam(defaultValue="0") int page) {return motion.list(deviceId,id,page);}
    @GetMapping("/windows/{id}") public Object detail(@PathVariable String deviceId,@PathVariable long id) {return motion.detail(deviceId,id);}
    @PatchMapping("/windows/{id}") public void label(@PathVariable String deviceId,@PathVariable long id,@RequestBody Label input) {motion.label(deviceId,id,input.label());}
    @DeleteMapping("/captures/{id}") public void delete(@PathVariable String deviceId,@PathVariable String id) {motion.delete(deviceId,id);}
    @GetMapping("/dataset.json") public ResponseEntity<Object> dataset(@PathVariable String deviceId) {return ResponseEntity.ok()
        .header("Content-Disposition","attachment; filename=\"du-lieu-ai.json\"").body(motion.dataset(deviceId));}
    @PostMapping("/model") public Object model(@PathVariable String deviceId,@RequestParam MultipartFile file) throws java.io.IOException {
        if(file.getSize()>1000000) throw new IllegalArgumentException("Tệp mô hình vượt giới hạn 1 MB.");
        String json=new String(file.getBytes(),java.nio.charset.StandardCharsets.UTF_8);
        var parsed=models.parse(json);
        if(!deviceId.equals(parsed.deviceId())) throw new IllegalArgumentException("Mô hình được huấn luyện cho thiết bị khác.");
        models.activate(json);return models.info();
    }
    @DeleteMapping("/model") public void removeModel(@PathVariable String deviceId) {models.remove();}
}
