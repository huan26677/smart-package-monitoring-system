package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record DeviceLocationResponse(

        String deviceId,

        Long scanId,

        Double latitude,

        Double longitude,

        Double accuracyMeters,

        String locationStatus,

        String locationSource,

        String locationLabel,

        String matchedBssid,

        Integer matchedRssi,

        long timestamp,

        Instant receivedAt

) {
}