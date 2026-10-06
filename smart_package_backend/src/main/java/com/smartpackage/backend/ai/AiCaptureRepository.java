package com.smartpackage.backend.ai;
import java.time.Instant;
import java.util.List;
import org.springframework.data.jpa.repository.JpaRepository;
public interface AiCaptureRepository extends JpaRepository<AiCapture,String> {
    List<AiCapture> findTop50ByDeviceIdOrderByCreatedAtDesc(String deviceId);
    boolean existsByDeviceIdAndStoppedFalseAndDeadlineAfter(String deviceId,Instant time);
}
