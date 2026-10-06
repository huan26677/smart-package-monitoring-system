package com.smartpackage.backend.service;

import java.time.Instant;
import java.util.List;
import java.util.Set;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.data.jpa.domain.Specification;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.transaction.annotation.Isolation;
import org.springframework.web.server.ResponseStatusException;
import com.smartpackage.backend.entity.PackageEventEntity;
import com.smartpackage.backend.repository.DeviceRepository;
import com.smartpackage.backend.repository.PackageEventRepository;
import com.smartpackage.backend.dto.response.PackageEventResponse;
import jakarta.persistence.EntityManager;

@Service
@Transactional(readOnly = true, isolation = Isolation.REPEATABLE_READ)
public class EventHistoryService {
    private final PackageEventRepository events;
    private final DeviceRepository devices;
    private final EntityManager entityManager;
    public EventHistoryService(PackageEventRepository events, DeviceRepository devices, EntityManager entityManager) {
        this.events = events; this.devices = devices; this.entityManager = entityManager;
    }
    public record History(List<PackageEventResponse> content, long totalElements, int totalPages, int page) {}
    public record Summary(long totalEvents, long impacts, long drops, long strongEvents,
            double maxG, long saturatedEvents) {}

    private Specification<PackageEventEntity> filter(String deviceId, String type, String level,
            Instant from, Instant to) {
        if (!devices.existsById(deviceId)) throw new ResponseStatusException(HttpStatus.NOT_FOUND, "Device not found");
        if (type != null && !Set.of("IMPACT", "DROP", "FREE_FALL", "TILT", "FLIP", "VIBRATION").contains(type))
            throw new IllegalArgumentException("Invalid event type");
        if (level != null && !Set.of("NONE", "LIGHT", "MEDIUM", "STRONG").contains(level))
            throw new IllegalArgumentException("Invalid event level");
        if (from != null && to != null && from.isAfter(to))
            throw new IllegalArgumentException("Start time must precede end time");
        return (root, query, cb) -> {
            var predicates = new java.util.ArrayList<jakarta.persistence.criteria.Predicate>();
            predicates.add(cb.equal(root.get("device").get("deviceId"), deviceId));
            if (type != null) predicates.add(cb.equal(root.get("type"), type));
            if (level != null) predicates.add(cb.equal(root.get("level"), level));
            if (from != null || to != null) predicates.add(cb.greaterThan(root.get("eventTimestamp"), 0L));
            if (from != null) predicates.add(cb.greaterThanOrEqualTo(root.get("eventTimestamp"), from.getEpochSecond()));
            if (to != null) predicates.add(cb.lessThanOrEqualTo(root.get("eventTimestamp"), to.getEpochSecond()));
            return cb.and(predicates.toArray(jakarta.persistence.criteria.Predicate[]::new));
        };
    }
    private Page<PackageEventEntity> page(Specification<PackageEventEntity> filter, int page, int size) {
        return events.findAll(filter, PageRequest.of(page, size,
                Sort.by(Sort.Order.desc("eventTimestamp"), Sort.Order.desc("id"))));
    }
    public History history(String deviceId, String type, String level, Instant from, Instant to, int page, int size) {
        if (page < 0 || size < 1 || size > 100) throw new IllegalArgumentException("Invalid page or size (1..100)");
        var result = page(filter(deviceId, type, level, from, to), page, size);
        return new History(result.getContent().stream().map(this::response).toList(),
                result.getTotalElements(), result.getTotalPages(), page);
    }
    public Summary summary(String deviceId, String type, String level, Instant from, Instant to) {
        var spec = filter(deviceId, type, level, from, to);
        var cb = entityManager.getCriteriaBuilder();
        var query = cb.createTupleQuery();
        var root = query.from(PackageEventEntity.class);
        query.multiselect(cb.count(root),
                cb.sum(cb.<Long>selectCase().when(cb.equal(root.get("type"), "IMPACT"), 1L).otherwise(0L)),
                cb.sum(cb.<Long>selectCase().when(cb.equal(root.get("type"), "DROP"), 1L).otherwise(0L)),
                cb.sum(cb.<Long>selectCase().when(cb.equal(root.get("level"), "STRONG"), 1L).otherwise(0L)),
                cb.max(root.<Double>get("gForce")),
                cb.sum(cb.<Long>selectCase().when(cb.isTrue(root.get("saturated")), 1L).otherwise(0L)));
        query.where(spec.toPredicate(root, query, cb));
        var row = entityManager.createQuery(query).getSingleResult();
        return new Summary(number(row.get(0)).longValue(), number(row.get(1)).longValue(),
                number(row.get(2)).longValue(), number(row.get(3)).longValue(),
                number(row.get(4)).doubleValue(), number(row.get(5)).longValue());
    }
    private Number number(Object value) { return value == null ? 0 : (Number) value; }
    public String csv(String deviceId, String type, String level, Instant from, Instant to) {
        var filter = filter(deviceId, type, level, from, to);
        if (events.count(filter) > 10000) throw new IllegalArgumentException("Narrow filters to at most 10000 events for export");
        StringBuilder csv = new StringBuilder("\uFEFFdevice_id,event_id,occurred_at,received_at,type,level,peak_g,duration_ms,saturated,angle,vibration\r\n");
        for (int index = 0; ; index++) {
            var result = page(filter, index, 500);
            for (var event : result) {
                csv.append(text(deviceId)).append(',').append(event.getEventId()).append(',')
                    .append(event.getEventTimestamp() > 0 ? Instant.ofEpochSecond(event.getEventTimestamp()) : "").append(',')
                    .append(event.getReceivedAt()).append(',').append(text(event.getType())).append(',')
                    .append(text(event.getLevel())).append(',').append(event.getGForce()).append(',')
                    .append(event.getDurationMs() == null ? "" : event.getDurationMs()).append(',')
                    .append(event.getSaturated() == null ? "" : event.getSaturated()).append(',')
                    .append(event.getAngle()).append(',').append(event.getVibration()).append("\r\n");
            }
            if (!result.hasNext()) break;
        }
        return csv.toString();
    }
    private String text(String value) {
        if (value == null) return "";
        if (value.matches("^[=+@\\-\\t\\r].*")) value = "'" + value;
        return "\"" + value.replace("\"", "\"\"") + "\"";
    }
    private PackageEventResponse response(PackageEventEntity e) {
        return new PackageEventResponse(e.getId(), e.getDevice().getDeviceId(), e.getEventId(),
                e.getType(), e.getLevel(), e.getGForce(), e.getAngle(), e.getVibration(), e.getUptimeMs(),
                e.getEventTimestamp(), e.getTimeText(), e.getReceivedAt(), e.getDurationMs(), e.getSaturated());
    }
}
