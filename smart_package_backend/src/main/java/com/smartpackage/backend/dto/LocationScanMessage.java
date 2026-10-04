package com.smartpackage.backend.dto;

import java.util.List;


public record LocationScanMessage(

        String deviceId,

        long timestamp,

        List<WifiAccessPointMessage>
                wifiAccessPoints

) {
}