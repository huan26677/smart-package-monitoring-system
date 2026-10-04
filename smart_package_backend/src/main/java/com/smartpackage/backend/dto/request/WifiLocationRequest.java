package com.smartpackage.backend.dto.request;

import jakarta.validation.constraints.DecimalMax;
import jakarta.validation.constraints.DecimalMin;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Positive;

public record WifiLocationRequest(

        @NotBlank(
                message = "BSSID is required"
        )
        @Pattern(
                regexp =
                        "(?i)^([0-9a-f]{2}:){5}[0-9a-f]{2}$",
                message =
                        "BSSID must use format AA:BB:CC:DD:EE:FF"
        )
        String bssid,

        @NotBlank(
                message = "Anchor name is required"
        )
        String name,

        @NotNull(
                message = "Latitude is required"
        )
        @DecimalMin(
                value = "-90.0",
                message = "Latitude must be >= -90"
        )
        @DecimalMax(
                value = "90.0",
                message = "Latitude must be <= 90"
        )
        Double latitude,

        @NotNull(
                message = "Longitude is required"
        )
        @DecimalMin(
                value = "-180.0",
                message = "Longitude must be >= -180"
        )
        @DecimalMax(
                value = "180.0",
                message = "Longitude must be <= 180"
        )
        Double longitude,

        @Positive(
                message =
                        "Radius must be greater than 0"
        )
        Double radiusMeters

) {
}
