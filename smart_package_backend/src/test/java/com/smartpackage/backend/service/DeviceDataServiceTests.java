package com.smartpackage.backend.service;

import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.assertEquals;

import static org.mockito.Mockito.when;

import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import org.junit.jupiter.api.extension.ExtendWith;

import org.mockito.Mock;

import org.mockito.junit.jupiter.MockitoExtension;

import com.smartpackage.backend.dto.response.DeviceDashboardResponse;
import com.smartpackage.backend.dto.response.DeviceLocationResponse;

import com.smartpackage.backend.entity.DeviceEntity;
import com.smartpackage.backend.entity.LocationScanEntity;

import com.smartpackage.backend.repository.DeviceRepository;
import com.smartpackage.backend.repository.LocationScanRepository;
import com.smartpackage.backend.repository.PackageEventRepository;
import com.smartpackage.backend.repository.TelemetryRepository;

@ExtendWith(MockitoExtension.class)
class DeviceDataServiceTests {

    @Mock
    private DeviceRepository
            deviceRepository;

    @Mock
    private TelemetryRepository
            telemetryRepository;

    @Mock
    private PackageEventRepository
            packageEventRepository;

    @Mock
    private LocationScanRepository
            locationScanRepository;

    private DeviceDataService
            deviceDataService;

    @BeforeEach
    void setUp() {

        deviceDataService =
                new DeviceDataService(

                        deviceRepository,

                        telemetryRepository,

                        packageEventRepository,

                        locationScanRepository
                );
    }

    /* =====================================================
     * DASHBOARD
     * ===================================================== */

    @Test
    void dashboardShouldMarkRecentlySeenDeviceOnline() {

        DeviceEntity device =
                new DeviceEntity(
                        "esp32-001"
                );

        when(
                deviceRepository
                        .findById(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        Optional.of(
                                device
                        )
                );

        when(
                telemetryRepository
                        .findFirstByDevice_DeviceIdOrderByIdDesc(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        Optional.empty()
                );

        when(
                packageEventRepository
                        .findFirstByDevice_DeviceIdOrderByIdDesc(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        Optional.empty()
                );

        when(
                locationScanRepository
                        .findFirstByDevice_DeviceIdOrderByIdDesc(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        Optional.empty()
                );

        DeviceDashboardResponse dashboard =
                deviceDataService
                        .getDashboard(
                                "esp32-001"
                        );

        assertTrue(
                dashboard.online()
        );

        assertEquals(
                "esp32-001",
                dashboard.deviceId()
        );

        assertNull(
                dashboard.latestTelemetry()
        );

        assertNull(
                dashboard.latestEvent()
        );

        assertNull(
                dashboard.latestLocation()
        );
    }

    /* =====================================================
     * LATEST LOCATION
     * ===================================================== */

    @Test
    void latestLocationShouldReturnNoAnchorInsteadOfOldLocation() {

        DeviceEntity device =
                new DeviceEntity(
                        "esp32-001"
                );

        LocationScanEntity scan =
                new LocationScanEntity(

                        device,

                        123456L
                );

        scan.markNoAnchor();

        when(
                deviceRepository
                        .existsById(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        true
                );

        when(
                locationScanRepository
                        .findFirstByDevice_DeviceIdOrderByIdDesc(
                                "esp32-001"
                        )
        )
                .thenReturn(
                        Optional.of(
                                scan
                        )
                );

        DeviceLocationResponse location =
                deviceDataService
                        .getLatestLocation(
                                "esp32-001"
                        );

        assertEquals(
                "NO_ANCHOR",
                location.locationStatus()
        );

        assertNull(
                location.latitude()
        );

        assertNull(
                location.longitude()
        );

        assertNull(
                location.locationLabel()
        );
    }
}
