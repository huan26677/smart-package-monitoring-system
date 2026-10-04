package com.smartpackage.backend.dto.geolocation;


public record GoogleWifiAccessPoint(

        String macAddress,

        int signalStrength

) {
}