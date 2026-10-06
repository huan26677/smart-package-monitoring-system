package com.smartpackage.backend.ai;
import java.time.Instant;
import java.util.Map;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import tools.jackson.databind.json.JsonMapper;
@Service
public class AiModelService {
    private final AiModelRepository repository;
    private final JsonMapper mapper;
    private volatile String cachedDocument;
    private volatile ForestModel cachedModel;
    public AiModelService(AiModelRepository repository,JsonMapper mapper) {this.repository=repository;this.mapper=mapper;}
    @Transactional(readOnly=true)
    public synchronized ForestModel current() {
        var saved=repository.findById(1);
        if(saved.isEmpty()) {cachedDocument=null;cachedModel=null;return null;}
        String json=saved.get().document;
        if(!json.equals(cachedDocument)) {var model=parse(json);cachedModel=model;cachedDocument=json;}
        return cachedModel;
    }
    public ForestModel parse(String json) {
        if(json==null || json.length()>1000000) throw new IllegalArgumentException("Tệp mô hình vượt giới hạn 1 MB.");
        try {var model=mapper.readValue(json,ForestModel.class);model.validate();model.validateEvaluation();return model;}
        catch(IllegalArgumentException e) {throw e;}
        catch(Exception e) {throw new IllegalArgumentException("Tệp JSON mô hình không đúng định dạng.");}
    }
    @Transactional
    public void activate(String json) {parse(json);repository.saveAndFlush(new AiModelEntity(json));}
    @Transactional
    public void remove() {repository.deleteById(1);}
    public Object info() {
        var model=current();
        return model==null?Map.of("available",false):Map.of("available",true,"version",model.version(),
                "trainedAt",model.trainedAt(),"deviceId",model.deviceId(),"classes",model.classes(),
                "scoreThreshold",model.scoreThreshold(),"evaluation",model.evaluation());
    }
}
