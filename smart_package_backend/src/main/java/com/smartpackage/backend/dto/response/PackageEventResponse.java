package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record PackageEventResponse(

        Long id,

        String deviceId,

        long eventId,

        String type,

        String level,

        double gForce,

        double angle,

        double vibration,

        long uptimeMs,

        long timestamp,

        String timeText,

        Instant receivedAt

) {
}