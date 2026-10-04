package com.smartpackage.backend.service;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.List;
import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import org.junit.jupiter.api.extension.ExtendWith;

import org.mockito.Mock;

import org.mockito.junit.jupiter.MockitoExtension;

import org.springframework.http.HttpStatus;
import org.springframework.web.server.ResponseStatusException;

import com.smartpackage.backend.dto.WifiAccessPointMessage;
import com.smartpackage.backend.dto.request.WifiLocationRequest;
import com.smartpackage.backend.dto.response.WifiLocationResponse;

import com.smartpackage.backend.entity.DeviceEntity;
import com.smartpackage.backend.entity.LocationScanEntity;
import com.smartpackage.backend.entity.WifiLocationEntity;

import com.smartpackage.backend.repository.LocationScanRepository;
import com.smartpackage.backend.repository.WifiLocationRepository;

@ExtendWith(MockitoExtension.class)
class WifiAnchorServiceTests {

    @Mock
    private WifiLocationRepository
            wifiLocationRepository;

    @Mock
    private LocationScanRepository
            locationScanRepository;

    private WifiAnchorService
            wifiAnchorService;

    @BeforeEach
    void setUp() {

        wifiAnchorService =
                new WifiAnchorService(

                        wifiLocationRepository,

                        locationScanRepository
                );
    }

    /* =====================================================
     * SAVE ANCHOR
     * ===================================================== */

    @Test
    void saveAnchorShouldNormalizeBssidAndUseDefaultRadius() {

        WifiLocationRequest request =
                new WifiLocationRequest(

                        "aa:bb:cc:dd:ee:ff",

                        "Kho A",

                        10.9800,

                        106.6700,

                        null
                );

        when(
                wifiLocationRepository
                        .findById(
                                "AA:BB:CC:DD:EE:FF"
                        )
        )
                .thenReturn(
                        Optional.empty()
                );

        when(
                wifiLocationRepository
                        .save(
                                any(
                                        WifiLocationEntity.class
                                )
                        )
        )
                .thenAnswer(
                        invocation ->
                                invocation.getArgument(
                                        0
                                )
                );

        WifiLocationResponse response =
                wifiAnchorService
                        .saveAnchor(
                                request
                        );

        assertEquals(
                "AA:BB:CC:DD:EE:FF",
                response.bssid()
        );

        assertEquals(
                "Kho A",
                response.name()
        );

        assertEquals(
                30.0,
                response.radiusMeters()
        );
    }

    /* =====================================================
     * STRONGEST ANCHOR
     * ===================================================== */

    @Test
    void resolveShouldChooseStrongestMatchedAnchor() {

        DeviceEntity device =
                new DeviceEntity(
                        "esp32-001"
                );

        LocationScanEntity scan =
                new LocationScanEntity(

                        device,

                        1000L
                );

        WifiLocationEntity khoA =
                new WifiLocationEntity(

                        "10:11:12:13:14:15",

                        "Kho A",

                        10.1,

                        106.1,

                        30
                );

        WifiLocationEntity cong =
                new WifiLocationEntity(

                        "20:21:22:23:24:25",

                        "Cong",

                        10.2,

                        106.2,

                        20
                );

        when(
                locationScanRepository
                        .findById(
                                1L
                        )
        )
                .thenReturn(
                        Optional.of(
                                scan
                        )
                );

        when(
                wifiLocationRepository
                        .findById(
                                "10:11:12:13:14:15"
                        )
        )
                .thenReturn(
                        Optional.of(
                                khoA
                        )
                );

        when(
                wifiLocationRepository
                        .findById(
                                "20:21:22:23:24:25"
                        )
        )
                .thenReturn(
                        Optional.of(
                                cong
                        )
                );

        wifiAnchorService
                .resolveAndStore(

                        1L,

                        List.of(

                                new WifiAccessPointMessage(

                                        "10:11:12:13:14:15",

                                        -70
                                ),

                                new WifiAccessPointMessage(

                                        "20:21:22:23:24:25",

                                        -40
                                )
                        )
                );

        assertEquals(
                "ANCHOR_MATCHED",
                scan.getLocationStatus()
        );

        assertEquals(
                "Cong",
                scan.getLocationLabel()
        );

        assertEquals(
                "20:21:22:23:24:25",
                scan.getMatchedBssid()
        );

        assertEquals(
                -40,
                scan.getMatchedRssi()
        );

        assertEquals(
                10.2,
                scan.getLatitude()
        );

        assertEquals(
                106.2,
                scan.getLongitude()
        );

        verify(
                locationScanRepository
        )
                .save(
                        scan
                );
    }

    /* =====================================================
     * NO ANCHOR
     * ===================================================== */

    @Test
    void resolveShouldMarkNoAnchorWhenNothingMatches() {

        DeviceEntity device =
                new DeviceEntity(
                        "esp32-001"
                );

        LocationScanEntity scan =
                new LocationScanEntity(

                        device,

                        1000L
                );

        when(
                locationScanRepository
                        .findById(
                                2L
                        )
        )
                .thenReturn(
                        Optional.of(
                                scan
                        )
                );

        when(
                wifiLocationRepository
                        .findById(
                                "30:31:32:33:34:35"
                        )
        )
                .thenReturn(
                        Optional.empty()
                );

        wifiAnchorService
                .resolveAndStore(

                        2L,

                        List.of(

                                new WifiAccessPointMessage(

                                        "30:31:32:33:34:35",

                                        -45
                                )
                        )
                );

        assertEquals(
                "NO_ANCHOR",
                scan.getLocationStatus()
        );

        assertNull(
                scan.getLatitude()
        );

        assertNull(
                scan.getLongitude()
        );

        assertNull(
                scan.getLocationLabel()
        );
    }

    /* =====================================================
     * DELETE
     * ===================================================== */

    @Test
    void deleteMissingAnchorShouldReturn404() {

        when(
                wifiLocationRepository
                        .existsById(
                                "AA:BB:CC:DD:EE:FF"
                        )
        )
                .thenReturn(
                        false
                );

        ResponseStatusException exception =
                assertThrows(

                        ResponseStatusException.class,

                        () ->
                                wifiAnchorService
                                        .deleteAnchor(
                                                "AA:BB:CC:DD:EE:FF"
                                        )
                );

        assertEquals(
                HttpStatus.NOT_FOUND.value(),

                exception
                        .getStatusCode()
                        .value()
        );
    }
}
