package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record DeviceLocationResponse(

        String deviceId,

        Long scanId,

        Double latitude,

        Double longitude,

        Double accuracyMeters,

        long timestamp,

        Instant receivedAt

) {
}