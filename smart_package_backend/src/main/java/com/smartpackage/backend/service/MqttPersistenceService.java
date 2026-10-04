package com.smartpackage.backend.service;

import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import com.smartpackage.backend.dto.LocationScanMessage;
import com.smartpackage.backend.dto.PackageEventMessage;
import com.smartpackage.backend.dto.TelemetryMessage;
import com.smartpackage.backend.dto.WifiAccessPointMessage;

import com.smartpackage.backend.entity.DeviceEntity;
import com.smartpackage.backend.entity.LocationScanEntity;
import com.smartpackage.backend.entity.PackageEventEntity;
import com.smartpackage.backend.entity.TelemetryEntity;
import com.smartpackage.backend.entity.WifiAccessPointEntity;

import com.smartpackage.backend.repository.DeviceRepository;
import com.smartpackage.backend.repository.LocationScanRepository;
import com.smartpackage.backend.repository.PackageEventRepository;
import com.smartpackage.backend.repository.TelemetryRepository;


@Service
public class MqttPersistenceService {


    private final DeviceRepository
            deviceRepository;

    private final TelemetryRepository
            telemetryRepository;

    private final PackageEventRepository
            packageEventRepository;

    private final LocationScanRepository
            locationScanRepository;


    public MqttPersistenceService(

            DeviceRepository
                    deviceRepository,

            TelemetryRepository
                    telemetryRepository,

            PackageEventRepository
                    packageEventRepository,

            LocationScanRepository
                    locationScanRepository

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
     * DEVICE
     * ===================================================== */

    private DeviceEntity getOrCreateDevice(
            String deviceId
    ) {

        if (
                deviceId == null ||
                deviceId.isBlank()
        ) {

            throw new IllegalArgumentException(
                    "deviceId is required"
            );
        }


        DeviceEntity device =
                deviceRepository
                        .findById(
                                deviceId
                        )
                        .orElseGet(
                                () ->
                                        deviceRepository.save(
                                                new DeviceEntity(
                                                        deviceId
                                                )
                                        )
                        );


        device.touch();


        return device;
    }


    /* =====================================================
     * TELEMETRY
     * ===================================================== */

    @Transactional
    public void saveTelemetry(
            TelemetryMessage data
    ) {

        DeviceEntity device =
                getOrCreateDevice(
                        data.deviceId()
                );


        TelemetryEntity telemetry =
                new TelemetryEntity(

                        device,

                        data.g(),

                        data.angle(),

                        data.vibration(),

                        data.state(),

                        data.rssi()
                );


        telemetryRepository.save(
                telemetry
        );
    }


    /* =====================================================
     * EVENT
     * ===================================================== */

    @Transactional
    public boolean saveEvent(
            PackageEventMessage data
    ) {

        DeviceEntity device =
                getOrCreateDevice(
                        data.deviceId()
                );


        boolean alreadyExists =
                packageEventRepository
                        .existsByDevice_DeviceIdAndEventId(

                                data.deviceId(),

                                data.eventId()
                        );


        if (alreadyExists) {

            return false;
        }


        PackageEventEntity event =
                new PackageEventEntity(

                        device,

                        data.eventId(),

                        data.type(),

                        data.level(),

                        data.g(),

                        data.angle(),

                        data.vibration(),

                        data.uptimeMs(),

                        data.timestamp(),

                        data.timeText()
                );


        packageEventRepository.save(
                event
        );


        return true;
    }


    /* =====================================================
     * LOCATION SCAN
     * ===================================================== */

    @Transactional
    public void saveLocationScan(
            LocationScanMessage data
    ) {

        DeviceEntity device =
                getOrCreateDevice(
                        data.deviceId()
                );


        LocationScanEntity locationScan =
                new LocationScanEntity(

                        device,

                        data.timestamp()
                );


        if (
                data.wifiAccessPoints()
                        != null
        ) {

            for (
                    WifiAccessPointMessage ap :
                    data.wifiAccessPoints()
            ) {

                WifiAccessPointEntity
                        accessPoint =
                                new WifiAccessPointEntity(

                                        ap.bssid(),

                                        ap.rssi()
                                );


                locationScan
                        .addWifiAccessPoint(
                                accessPoint
                        );
            }
        }


        locationScanRepository.save(
                locationScan
        );
    }
}