package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record WifiLocationResponse(

        String bssid,

        String name,

        double latitude,

        double longitude,

        double radiusMeters,

        Instant createdAt

) {
}