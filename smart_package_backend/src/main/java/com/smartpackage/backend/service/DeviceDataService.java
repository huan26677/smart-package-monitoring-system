package com.smartpackage.backend.service;

import java.util.List;

import org.springframework.data.domain.PageRequest;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import com.smartpackage.backend.dto.response.DeviceResponse;
import com.smartpackage.backend.dto.response.LocationScanResponse;
import com.smartpackage.backend.dto.response.PackageEventResponse;
import com.smartpackage.backend.dto.response.TelemetryResponse;
import com.smartpackage.backend.dto.response.WifiAccessPointResponse;

import com.smartpackage.backend.entity.DeviceEntity;
import com.smartpackage.backend.entity.LocationScanEntity;
import com.smartpackage.backend.entity.PackageEventEntity;
import com.smartpackage.backend.entity.TelemetryEntity;

import com.smartpackage.backend.repository.DeviceRepository;
import com.smartpackage.backend.repository.LocationScanRepository;
import com.smartpackage.backend.repository.PackageEventRepository;
import com.smartpackage.backend.repository.TelemetryRepository;
import com.smartpackage.backend.dto.response.DeviceLocationResponse;

@Service
public class DeviceDataService {


    private final DeviceRepository
            deviceRepository;

    private final TelemetryRepository
            telemetryRepository;

    private final PackageEventRepository
            packageEventRepository;

    private final LocationScanRepository
            locationScanRepository;


    public DeviceDataService(

            DeviceRepository deviceRepository,

            TelemetryRepository telemetryRepository,

            PackageEventRepository packageEventRepository,

            LocationScanRepository locationScanRepository

    ) {

        this.deviceRepository =
                deviceRepository;

        this.telemetryRepository =
                telemetryRepository;

        this.packageEventRepository =
                packageEventRepository;

        this.locationScanRepository =
                locationScanRepository;
    }


    /* =====================================================
     * DEVICES
     * ===================================================== */

    @Transactional(readOnly = true)
    public List<DeviceResponse>
            getDevices() {

        return deviceRepository
                .findAllByOrderByDeviceIdAsc()
                .stream()
                .map(
                        this::toDeviceResponse
                )
                .toList();
    }


    @Transactional(readOnly = true)
    public DeviceResponse getDevice(
            String deviceId
    ) {

        DeviceEntity device =
                getDeviceEntity(
                        deviceId
                );


        return toDeviceResponse(
                device
        );
    }


    /* =====================================================
     * TELEMETRY
     * ===================================================== */

    @Transactional(readOnly = true)
    public TelemetryResponse
            getLatestTelemetry(
                    String deviceId
            ) {

        ensureDeviceExists(
                deviceId
        );


        TelemetryEntity telemetry =
                telemetryRepository
                        .findFirstByDevice_DeviceIdOrderByIdDesc(
                                deviceId
                        )
                        .orElseThrow(
                                () ->
                                        new ResponseStatusException(
                                                HttpStatus.NOT_FOUND,
                                                "Telemetry not found"
                                        )
                        );


        return toTelemetryResponse(
                telemetry
        );
    }


    @Transactional(readOnly = true)
    public List<TelemetryResponse>
            getTelemetry(
                    String deviceId,
                    int limit
            ) {

        ensureDeviceExists(
                deviceId
        );


        int safeLimit =
                normalizeLimit(
                        limit,
                        100
                );


        return telemetryRepository
                .findByDevice_DeviceIdOrderByIdDesc(

                        deviceId,

                        PageRequest.of(
                                0,
                                safeLimit
                        )
                )
                .stream()
                .map(
                        this::toTelemetryResponse
                )
                .toList();
    }


    /* =====================================================
     * EVENTS
     * ===================================================== */

    @Transactional(readOnly = true)
    public List<PackageEventResponse>
            getEvents(
                    String deviceId,
                    int limit
            ) {

        ensureDeviceExists(
                deviceId
        );


        int safeLimit =
                normalizeLimit(
                        limit,
                        50
                );


        return packageEventRepository
                .findByDevice_DeviceIdOrderByIdDesc(

                        deviceId,

                        PageRequest.of(
                                0,
                                safeLimit
                        )
                )
                .stream()
                .map(
                        this::toEventResponse
                )
                .toList();
    }


    /* =====================================================
     * LOCATION SCANS
     * ===================================================== */

    @Transactional(readOnly = true)
    public List<LocationScanResponse>
            getLocationScans(
                    String deviceId,
                    int limit
            ) {

        ensureDeviceExists(
                deviceId
        );


        int safeLimit =
                normalizeLimit(
                        limit,
                        20
                );


        return locationScanRepository
                .findByDevice_DeviceIdOrderByIdDesc(

                        deviceId,

                        PageRequest.of(
                                0,
                                safeLimit
                        )
                )
                .stream()
                .map(
                        this::toLocationScanResponse
                )
                .toList();
    }

        @Transactional(readOnly = true)
        public DeviceLocationResponse
                getLatestLocation(
                        String deviceId
                ) {

        ensureDeviceExists(
                deviceId
        );


        LocationScanEntity scan =
                locationScanRepository
                        .findFirstByDevice_DeviceIdAndLatitudeIsNotNullAndLongitudeIsNotNullOrderByIdDesc(
                                deviceId
                        )
                        .orElseThrow(
                                () ->
                                        new ResponseStatusException(
                                                HttpStatus.NOT_FOUND,
                                                "Location not found"
                                        )
                        );


        return new DeviceLocationResponse(

                deviceId,

                scan.getId(),

                scan.getLatitude(),

                scan.getLongitude(),

                scan.getAccuracyMeters(),

                scan.getLocationSource(),

                scan.getLocationLabel(),

                scan.getMatchedBssid(),

                scan.getMatchedRssi(),

                scan.getScanTimestamp(),

                scan.getReceivedAt()
                );
        }
    /* =====================================================
     * HELPERS
     * ===================================================== */

    private void ensureDeviceExists(
            String deviceId
    ) {

        if (
                !deviceRepository.existsById(
                        deviceId
                )
        ) {

            throw new ResponseStatusException(
                    HttpStatus.NOT_FOUND,
                    "Device not found"
            );
        }
    }


    private DeviceEntity getDeviceEntity(
            String deviceId
    ) {

        return deviceRepository
                .findById(
                        deviceId
                )
                .orElseThrow(
                        () ->
                                new ResponseStatusException(
                                        HttpStatus.NOT_FOUND,
                                        "Device not found"
                                )
                );
    }


    private int normalizeLimit(
            int limit,
            int defaultLimit
    ) {

        if (limit <= 0) {

            return defaultLimit;
        }


        return Math.min(
                limit,
                500
        );
    }


    /* =====================================================
     * MAPPING
     * ===================================================== */

    private DeviceResponse toDeviceResponse(
            DeviceEntity entity
    ) {

        return new DeviceResponse(

                entity.getDeviceId(),

                entity.getCreatedAt(),

                entity.getLastSeenAt()
        );
    }


    private TelemetryResponse
            toTelemetryResponse(
                    TelemetryEntity entity
            ) {

        return new TelemetryResponse(

                entity.getId(),

                entity
                        .getDevice()
                        .getDeviceId(),

                entity.getGForce(),

                entity.getAngle(),

                entity.getVibration(),

                entity.getState(),

                entity.getWifiRssi(),

                entity.getReceivedAt()
        );
    }


    private PackageEventResponse
            toEventResponse(
                    PackageEventEntity entity
            ) {

        return new PackageEventResponse(

                entity.getId(),

                entity
                        .getDevice()
                        .getDeviceId(),

                entity.getEventId(),

                entity.getType(),

                entity.getLevel(),

                entity.getGForce(),

                entity.getAngle(),

                entity.getVibration(),

                entity.getUptimeMs(),

                entity.getEventTimestamp(),

                entity.getTimeText(),

                entity.getReceivedAt()
        );
    }


    private LocationScanResponse
            toLocationScanResponse(
                    LocationScanEntity entity
            ) {

        List<WifiAccessPointResponse> aps =
                entity
                        .getWifiAccessPoints()
                        .stream()
                        .map(
                                ap ->
                                        new WifiAccessPointResponse(

                                                ap.getId(),

                                                ap.getBssid(),

                                                ap.getRssi()
                                        )
                        )
                        .toList();


        return new LocationScanResponse(

                entity.getId(),

                entity
                        .getDevice()
                        .getDeviceId(),

                entity.getScanTimestamp(),

                entity.getReceivedAt(),

                entity.getLatitude(),

                entity.getLongitude(),

                entity.getAccuracyMeters(),

                entity.getLocationStatus(),

                entity.getLocationSource(),

                entity.getLocationLabel(),

                entity.getMatchedBssid(),

                entity.getMatchedRssi(),

                aps
        );
    }
}