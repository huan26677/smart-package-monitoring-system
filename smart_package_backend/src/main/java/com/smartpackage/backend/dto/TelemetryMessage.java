package com.smartpackage.backend.dto;


public record TelemetryMessage(

        String deviceId,

        double g,

        double angle,

        double vibration,

        String state,

        int rssi

) {
}