package com.smartpackage.backend.dto;


public record PackageEventMessage(

        String deviceId,

        long eventId,

        String type,

        String level,

        double g,

        double angle,

        double vibration,

        long uptimeMs,

        long timestamp,

        String timeText,

        Long durationMs,

        Boolean saturated

) {
}