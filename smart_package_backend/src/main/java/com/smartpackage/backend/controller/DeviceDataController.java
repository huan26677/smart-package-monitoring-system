package com.smartpackage.backend.controller;

import java.util.List;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.smartpackage.backend.dto.response.DeviceResponse;
import com.smartpackage.backend.dto.response.LocationScanResponse;
import com.smartpackage.backend.dto.response.PackageEventResponse;
import com.smartpackage.backend.dto.response.TelemetryResponse;

import com.smartpackage.backend.service.DeviceDataService;
import com.smartpackage.backend.dto.response.DeviceLocationResponse;

@RestController
@RequestMapping("/api/devices")
public class DeviceDataController {


    private final DeviceDataService
            deviceDataService;


    public DeviceDataController(
            DeviceDataService deviceDataService
    ) {

        this.deviceDataService =
                deviceDataService;
    }


    /* =====================================================
     * DEVICES
     * ===================================================== */

    @GetMapping
    public List<DeviceResponse>
            getDevices() {

        return deviceDataService
                .getDevices();
    }


    @GetMapping("/{deviceId}")
    public DeviceResponse getDevice(

            @PathVariable
            String deviceId

    ) {

        return deviceDataService
                .getDevice(
                        deviceId
                );
    }


    /* =====================================================
     * TELEMETRY
     * ===================================================== */

    @GetMapping(
            "/{deviceId}/telemetry/latest"
    )
    public TelemetryResponse
            getLatestTelemetry(

                    @PathVariable
                    String deviceId

            ) {

        return deviceDataService
                .getLatestTelemetry(
                        deviceId
                );
    }


    @GetMapping(
            "/{deviceId}/telemetry"
    )
    public List<TelemetryResponse>
            getTelemetry(

                    @PathVariable
                    String deviceId,

                    @RequestParam(
                            defaultValue = "100"
                    )
                    int limit

            ) {

        return deviceDataService
                .getTelemetry(
                        deviceId,
                        limit
                );
    }


    /* =====================================================
     * EVENTS
     * ===================================================== */

    @GetMapping(
            "/{deviceId}/events"
    )
    public List<PackageEventResponse>
            getEvents(

                    @PathVariable
                    String deviceId,

                    @RequestParam(
                            defaultValue = "50"
                    )
                    int limit

            ) {

        return deviceDataService
                .getEvents(
                        deviceId,
                        limit
                );
    }


    /* =====================================================
     * LOCATION SCANS
     * ===================================================== */

    @GetMapping(
            "/{deviceId}/location-scans"
    )
    public List<LocationScanResponse>
            getLocationScans(

                    @PathVariable
                    String deviceId,

                    @RequestParam(
                            defaultValue = "20"
                    )
                    int limit

            ) {

        return deviceDataService
                .getLocationScans(
                        deviceId,
                        limit
                );
    }
        @GetMapping(
                "/{deviceId}/location/latest"
        )
        public DeviceLocationResponse
                getLatestLocation(

                        @PathVariable
                        String deviceId

                ) {

        return deviceDataService
                .getLatestLocation(
                        deviceId
                );
        }
}