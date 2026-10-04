package com.smartpackage.backend.dto;


public record WifiAccessPointMessage(

        String bssid,

        int rssi

) {
}