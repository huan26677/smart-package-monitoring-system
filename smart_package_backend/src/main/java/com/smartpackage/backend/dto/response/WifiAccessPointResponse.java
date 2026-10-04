package com.smartpackage.backend.dto.response;


public record WifiAccessPointResponse(

        Long id,

        String bssid,

        int rssi

) {
}