package com.smartpackage.backend.dto.geolocation;


public record GoogleGeolocationResponse(

        GoogleLocation location,

        Double accuracy

) {
}