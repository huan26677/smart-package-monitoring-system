package com.smartpackage.backend.dto.response;

import java.time.Instant;


public record DeviceResponse(

        String deviceId,

        Instant createdAt,

        Instant lastSeenAt

) {
}