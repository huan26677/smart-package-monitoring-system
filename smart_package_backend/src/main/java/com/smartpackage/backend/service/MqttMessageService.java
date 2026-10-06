package com.smartpackage.backend.service;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import tools.jackson.databind.json.JsonMapper;
import com.smartpackage.backend.dto.LocationScanMessage;
import com.smartpackage.backend.dto.PackageEventMessage;
import com.smartpackage.backend.dto.TelemetryMessage;

@Service
public class MqttMessageService {
    private static final Logger logger = LoggerFactory.getLogger(MqttMessageService.class);
    private final JsonMapper jsonMapper;
    private final MqttPersistenceService persistence;
    private final WifiAnchorService anchors;

    public record EventReceipt(String deviceId, long eventId) {}

    public MqttMessageService(JsonMapper jsonMapper, MqttPersistenceService persistence,
            WifiAnchorService anchors) {
        this.jsonMapper = jsonMapper;
        this.persistence = persistence;
        this.anchors = anchors;
    }

    public EventReceipt handle(String topic, String payload) {
        try {
            if (topic.endsWith("/telemetry")) {
                TelemetryMessage data = jsonMapper.readValue(payload, TelemetryMessage.class);
                validateDevice(topic, data.deviceId(), "telemetry");
                persistence.saveTelemetry(data);
            } else if (topic.endsWith("/event")) {
                PackageEventMessage data = jsonMapper.readValue(payload, PackageEventMessage.class);
                validateDevice(topic, data.deviceId(), "event");
                if (data.eventId() <= 0 || data.timestamp() < 0 || data.uptimeMs() < 0
                        || !Double.isFinite(data.g()) || data.g() < 0
                        || !Double.isFinite(data.angle()) || !Double.isFinite(data.vibration())
                        || (data.durationMs() != null && data.durationMs() < 0)
                        || data.type() == null || !java.util.Set.of("IMPACT", "DROP", "FREE_FALL",
                                "TILT", "FLIP", "VIBRATION").contains(data.type())
                        || data.level() == null || !java.util.Set.of("NONE", "LIGHT", "MEDIUM",
                                "STRONG").contains(data.level())) {
                    throw new IllegalArgumentException("Invalid event measurement");
                }
                // This proxied transaction returns only after commit. Duplicates also get receipts.
                boolean saved = persistence.saveEvent(data);
                logger.info("Event {} / {}: {}", data.deviceId(), data.eventId(),
                        saved ? "saved" : "already saved");
                return new EventReceipt(data.deviceId(), data.eventId());
            } else if (topic.endsWith("/location-scan")) {
                LocationScanMessage data = jsonMapper.readValue(payload, LocationScanMessage.class);
                validateDevice(topic, data.deviceId(), "location-scan");
                Long scanId = persistence.saveLocationScan(data);
                anchors.resolveAndStore(scanId, data.wifiAccessPoints());
            }
        } catch (Exception exception) {
            // No receipt on parse/validation/database failure: device keeps its durable copy.
            logger.error("MQTT processing failed for {}", topic, exception);
        }
        return null;
    }

    private void validateDevice(String topic, String deviceId, String kind) {
        if (deviceId == null || !deviceId.matches("[A-Za-z0-9_-]{1,64}")
                || !topic.equals("smart-package/" + deviceId + "/" + kind)) {
            throw new IllegalArgumentException("Device ID does not match MQTT topic");
        }
    }
}
