package com.smartpackage.backend.ai;
import java.util.List;
import org.springframework.data.domain.*;
import org.springframework.data.jpa.repository.*;
public interface AiWindowRepository extends JpaRepository<AiWindowEntity,Long> {
    boolean existsByCapture_IdAndSequence(String id,int sequence);
    long countByCapture_Id(String id);
    Page<AiWindowEntity> findByCapture_IdOrderBySequenceAsc(String id,Pageable page);
    @Query("select w.label, count(w), count(distinct w.capture.groupName) from AiWindowEntity w where w.capture.deviceId=:deviceId and w.timingValid=true and w.saturated=false group by w.label")
    List<Object[]> labelCounts(String deviceId);
    @Query("select w from AiWindowEntity w join fetch w.capture where w.capture.deviceId=:deviceId and w.label<>'UNLABELED' and w.timingValid=true and w.saturated=false order by w.id")
    List<AiWindowEntity> dataset(String deviceId,Pageable limit);
    void deleteByCapture_Id(String id);
}
