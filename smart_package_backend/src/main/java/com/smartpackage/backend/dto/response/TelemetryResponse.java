package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record TelemetryResponse(

        Long id,

        String deviceId,

        double gForce,

        double angle,

        double vibration,

        String state,

        int wifiRssi,

        Instant receivedAt,
        Integer pendingEvents,
        Long rejectedEvents

) {
}