package com.smartpackage.backend.dto.request;


public record WifiLocationRequest(

        String bssid,

        String name,

        double latitude,

        double longitude,

        double radiusMeters

) {
}