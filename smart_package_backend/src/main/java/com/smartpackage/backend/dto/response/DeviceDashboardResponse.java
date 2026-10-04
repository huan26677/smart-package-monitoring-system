package com.smartpackage.backend.dto.response;

import java.time.Instant;

public record DeviceDashboardResponse(

        String deviceId,

        boolean online,

        long secondsSinceLastSeen,

        Instant lastSeenAt,

        TelemetryResponse latestTelemetry,

        PackageEventResponse latestEvent,

        DeviceLocationResponse latestLocation

) {
}
