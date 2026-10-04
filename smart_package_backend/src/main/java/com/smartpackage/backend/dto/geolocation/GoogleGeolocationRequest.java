package com.smartpackage.backend.dto.geolocation;

import java.util.List;


public record GoogleGeolocationRequest(

        boolean considerIp,

        List<GoogleWifiAccessPoint>
                wifiAccessPoints

) {
}