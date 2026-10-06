package com.smartpackage.backend.ai;
import java.time.Instant;
import jakarta.persistence.*;
@Entity @Table(name="ai_captures",indexes=@Index(name="idx_ai_capture_device",columnList="device_id,created_at"))
public class AiCapture {
    @Id public String id;
    @Column(name="device_id",nullable=false,length=64) public String deviceId;
    @Column(nullable=false,length=120) public String name;
    @Column(nullable=false,length=120) public String groupName;
    @Column(name="created_at",nullable=false) public Instant createdAt;
    @Column(nullable=false) public Instant deadline;
    public int expectedWindows;
    public boolean stopped;
    protected AiCapture() {}
    public String getDeviceId() {return deviceId;}
    public AiCapture(String deviceId,String name,String groupName,int duration) {
        this.id=java.util.UUID.randomUUID().toString();this.deviceId=deviceId;this.name=name;this.groupName=groupName;
        this.createdAt=Instant.now();this.deadline=createdAt.plusSeconds(duration+45);this.expectedWindows=duration/2;
    }
}
