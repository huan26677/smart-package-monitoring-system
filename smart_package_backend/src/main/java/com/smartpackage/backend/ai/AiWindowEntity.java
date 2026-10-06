package com.smartpackage.backend.ai;
import java.time.Instant;
import jakarta.persistence.*;
@Entity @Table(name="ai_windows",uniqueConstraints=@UniqueConstraint(name="uk_ai_session_window",columnNames={"capture_id","sequence"}))
public class AiWindowEntity {
    @Id @GeneratedValue(strategy=GenerationType.IDENTITY) public Long id;
    @ManyToOne(fetch=FetchType.LAZY,optional=false) @JoinColumn(name="capture_id",nullable=false) public AiCapture capture;
    @Column(nullable=false) public int sequence;
    @Column(nullable=false,length=20) public String label="UNLABELED";
    @Column(columnDefinition="text",nullable=false) public String samplesJson;
    @Column(columnDefinition="text",nullable=false) public String featuresJson;
    public boolean timingValid;
    public boolean saturated;
    public long timestamp;
    @Column(nullable=false) public Instant receivedAt=Instant.now();
    @Column(length=20,nullable=false) public String ruleLabel;
    protected AiWindowEntity() {}
    public AiWindowEntity(AiCapture capture,MotionWindow data,String samples,String features,MotionFeatures.Result result) {
        this.capture=capture;this.sequence=data.sessionWindow();this.timestamp=data.timestamp();
        this.samplesJson=samples;this.featuresJson=features;this.timingValid=result.timingValid();
        this.saturated=result.saturated();this.ruleLabel=data.ruleLabel();
    }
}
