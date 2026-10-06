package com.smartpackage.backend.ai;
import java.time.Instant;
import jakarta.persistence.*;
@Entity @Table(name="ai_model")
public class AiModelEntity {
    @Id public int id=1;
    @Column(columnDefinition="text",nullable=false) public String document;
    public Instant importedAt;
    protected AiModelEntity() {}
    public AiModelEntity(String document) {this.document=document;this.importedAt=Instant.now();}
}
