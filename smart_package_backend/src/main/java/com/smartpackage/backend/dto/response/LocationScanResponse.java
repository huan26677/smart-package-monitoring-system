package com.smartpackage.backend.dto.response;

import java.time.Instant;
import java.util.List;


public record LocationScanResponse(

        Long id,

        String deviceId,

        long timestamp,

        Instant receivedAt,

        Double latitude,

        Double longitude,

        Double accuracyMeters,

        String locationStatus,

        String locationSource,

        String locationLabel,

        String matchedBssid,

        Integer matchedRssi,

        List<WifiAccessPointResponse>
                wifiAccessPoints

) {
}
