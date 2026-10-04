package com.smartpackage.backend.service;

import java.util.Comparator;
import java.util.List;

import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import com.smartpackage.backend.dto.WifiAccessPointMessage;
import com.smartpackage.backend.dto.request.WifiLocationRequest;
import com.smartpackage.backend.dto.response.WifiLocationResponse;

import com.smartpackage.backend.entity.LocationScanEntity;
import com.smartpackage.backend.entity.WifiLocationEntity;

import com.smartpackage.backend.repository.LocationScanRepository;
import com.smartpackage.backend.repository.WifiLocationRepository;


@Service
public class WifiAnchorService {


    private final WifiLocationRepository
            wifiLocationRepository;

    private final LocationScanRepository
            locationScanRepository;


    public WifiAnchorService(

            WifiLocationRepository
                    wifiLocationRepository,

            LocationScanRepository
                    locationScanRepository

    ) {

        this.wifiLocationRepository =
                wifiLocationRepository;

        this.locationScanRepository =
                locationScanRepository;
    }


    /* =====================================================
     * REGISTER / UPDATE ANCHOR
     * ===================================================== */

    @Transactional
    public WifiLocationResponse saveAnchor(
            WifiLocationRequest request
    ) {

        String bssid =
                normalizeBssid(
                        request.bssid()
                );


        if (
                request.name() == null ||
                request.name().isBlank()
        ) {

            throw new IllegalArgumentException(
                    "Anchor name is required"
            );
        }


        if (
                request.latitude() < -90 ||
                request.latitude() > 90
        ) {

            throw new IllegalArgumentException(
                    "Invalid latitude"
            );
        }


        if (
                request.longitude() < -180 ||
                request.longitude() > 180
        ) {

            throw new IllegalArgumentException(
                    "Invalid longitude"
            );
        }


        double radius =
                request.radiusMeters() > 0
                        ? request.radiusMeters()
                        : 30.0;


        WifiLocationEntity entity =
                wifiLocationRepository
                        .findById(
                                bssid
                        )
                        .orElseGet(
                                () ->
                                        new WifiLocationEntity(

                                                bssid,

                                                request.name(),

                                                request.latitude(),

                                                request.longitude(),

                                                radius
                                        )
                        );


        entity.update(

                request.name(),

                request.latitude(),

                request.longitude(),

                radius
        );


        WifiLocationEntity saved =
                wifiLocationRepository.save(
                        entity
                );


        return toResponse(
                saved
        );
    }


    /* =====================================================
     * LIST ANCHORS
     * ===================================================== */

    @Transactional(readOnly = true)
    public List<WifiLocationResponse>
            getAnchors() {

        return wifiLocationRepository
                .findAll()
                .stream()
                .sorted(
                        Comparator.comparing(
                                WifiLocationEntity::getName
                        )
                )
                .map(
                        this::toResponse
                )
                .toList();
    }


    /* =====================================================
     * DELETE ANCHOR
     * ===================================================== */

    @Transactional
    public void deleteAnchor(
            String bssid
    ) {

        wifiLocationRepository
                .deleteById(
                        normalizeBssid(
                                bssid
                        )
                );
    }


    /* =====================================================
     * RESOLVE LOCATION
     * ===================================================== */

    @Transactional
    public void resolveAndStore(

            Long scanId,

            List<WifiAccessPointMessage>
                    wifiAccessPoints

    ) {

        LocationScanEntity scan =
                locationScanRepository
                        .findById(
                                scanId
                        )
                        .orElse(null);


        if (scan == null) {

            return;
        }


        if (
                wifiAccessPoints == null ||
                wifiAccessPoints.isEmpty()
        ) {

            scan.markNoAnchor();

            locationScanRepository.save(
                    scan
            );

            return;
        }


        WifiAccessPointMessage bestAp =
                null;

        WifiLocationEntity bestAnchor =
                null;


        for (
                WifiAccessPointMessage ap :
                wifiAccessPoints
        ) {

            String bssid;

            try {

                bssid =
                        normalizeBssid(
                                ap.bssid()
                        );

            }
            catch (IllegalArgumentException e) {

                continue;
            }


            WifiLocationEntity anchor =
                    wifiLocationRepository
                            .findById(
                                    bssid
                            )
                            .orElse(null);


            if (anchor == null) {

                continue;
            }


            /*
             * RSSI cang lon thi song cang manh.
             * -40 manh hon -70.
             */
            if (
                    bestAp == null ||
                    ap.rssi() > bestAp.rssi()
            ) {

                bestAp =
                        ap;

                bestAnchor =
                        anchor;
            }
        }


        if (
                bestAp == null ||
                bestAnchor == null
        ) {

            scan.markNoAnchor();

            locationScanRepository.save(
                    scan
            );

            return;
        }


        scan.markAnchorMatched(

                bestAnchor.getName(),

                bestAnchor.getBssid(),

                bestAp.rssi(),

                bestAnchor.getLatitude(),

                bestAnchor.getLongitude(),

                bestAnchor.getRadiusMeters()
        );


        locationScanRepository.save(
                scan
        );


        System.out.println(
                "[WIFI ANCHOR] MATCHED"
                + " | scanId="
                + scanId
                + " | anchor="
                + bestAnchor.getName()
                + " | bssid="
                + bestAnchor.getBssid()
                + " | rssi="
                + bestAp.rssi()
        );
    }


    /* =====================================================
     * HELPERS
     * ===================================================== */

    private String normalizeBssid(
            String bssid
    ) {

        if (
                bssid == null ||
                !bssid.matches(
                        "(?i)^([0-9a-f]{2}:){5}[0-9a-f]{2}$"
                )
        ) {

            throw new IllegalArgumentException(
                    "Invalid BSSID"
            );
        }


        return bssid.toUpperCase();
    }


    private WifiLocationResponse toResponse(
            WifiLocationEntity entity
    ) {

        return new WifiLocationResponse(

                entity.getBssid(),

                entity.getName(),

                entity.getLatitude(),

                entity.getLongitude(),

                entity.getRadiusMeters(),

                entity.getCreatedAt()
        );
    }
}