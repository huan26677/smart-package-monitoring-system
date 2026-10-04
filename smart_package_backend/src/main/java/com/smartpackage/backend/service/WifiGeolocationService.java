package com.smartpackage.backend.service;

import java.net.URI;
import java.net.URLEncoder;

import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

import java.nio.charset.StandardCharsets;

import java.time.Duration;

import java.util.ArrayList;
import java.util.List;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;

import com.smartpackage.backend.dto.WifiAccessPointMessage;
import com.smartpackage.backend.dto.geolocation.GoogleGeolocationRequest;
import com.smartpackage.backend.dto.geolocation.GoogleGeolocationResponse;
import com.smartpackage.backend.dto.geolocation.GoogleWifiAccessPoint;

import com.smartpackage.backend.entity.LocationScanEntity;

import com.smartpackage.backend.repository.LocationScanRepository;

import tools.jackson.databind.json.JsonMapper;


@Service
public class WifiGeolocationService {


    private static final String GOOGLE_URL =
            "https://www.googleapis.com/geolocation/v1/geolocate?key=";


    private final JsonMapper jsonMapper;

    private final LocationScanRepository
            locationScanRepository;

    private final boolean enabled;

    private final String apiKey;

    private final HttpClient httpClient;


    public WifiGeolocationService(

            JsonMapper jsonMapper,

            LocationScanRepository
                    locationScanRepository,

            @Value("${app.geolocation.enabled:false}")
            boolean enabled,

            @Value("${app.geolocation.google-api-key:}")
            String apiKey,

            @Value("${app.geolocation.timeout-seconds:10}")
            int timeoutSeconds

    ) {

        this.jsonMapper =
                jsonMapper;

        this.locationScanRepository =
                locationScanRepository;

        this.enabled =
                enabled;

        this.apiKey =
                apiKey;


        this.httpClient =
                HttpClient
                        .newBuilder()
                        .connectTimeout(
                                Duration.ofSeconds(
                                        timeoutSeconds
                                )
                        )
                        .build();
    }


    @Async
    public void resolveAndStore(

            Long scanId,

            List<WifiAccessPointMessage>
                    wifiAccessPoints

    ) {

        if (
                !enabled ||
                apiKey == null ||
                apiKey.isBlank()
        ) {

            updateStatus(
                    scanId,
                    "DISABLED"
            );

            return;
        }


        List<GoogleWifiAccessPoint>
                usableAccessPoints =
                        new ArrayList<>();


        if (wifiAccessPoints != null) {

            for (
                    WifiAccessPointMessage ap :
                    wifiAccessPoints
            ) {

                if (
                        isUsableBssid(
                                ap.bssid()
                        )
                ) {

                    usableAccessPoints.add(

                            new GoogleWifiAccessPoint(

                                    ap.bssid(),

                                    ap.rssi()
                            )
                    );
                }
            }
        }


        /*
         * Google WiFi geolocation
         * can toi thieu 2 AP hop le.
         */
        if (
                usableAccessPoints.size()
                        < 2
        ) {

            updateStatus(
                    scanId,
                    "INSUFFICIENT_WIFI"
            );

            return;
        }


        try {

            GoogleGeolocationRequest requestData =
                    new GoogleGeolocationRequest(

                            false,

                            usableAccessPoints
                    );


            String requestJson =
                    jsonMapper
                            .writeValueAsString(
                                    requestData
                            );


            String encodedKey =
                    URLEncoder.encode(

                            apiKey,

                            StandardCharsets.UTF_8
                    );


            HttpRequest request =
                    HttpRequest
                            .newBuilder()
                            .uri(
                                    URI.create(
                                            GOOGLE_URL
                                            + encodedKey
                                    )
                            )
                            .timeout(
                                    Duration.ofSeconds(
                                            10
                                    )
                            )
                            .header(
                                    "Content-Type",
                                    "application/json"
                            )
                            .POST(
                                    HttpRequest
                                            .BodyPublishers
                                            .ofString(
                                                    requestJson,
                                                    StandardCharsets.UTF_8
                                            )
                            )
                            .build();


            HttpResponse<String> response =
                    httpClient.send(

                            request,

                            HttpResponse
                                    .BodyHandlers
                                    .ofString(
                                            StandardCharsets.UTF_8
                                    )
                    );


            if (
                    response.statusCode()
                            == 404
            ) {

                updateStatus(
                        scanId,
                        "NOT_FOUND"
                );

                return;
            }


            if (
                    response.statusCode() < 200 ||
                    response.statusCode() >= 300
            ) {

                System.err.println(
                        "[GEOLOCATION] Google API HTTP "
                                + response.statusCode()
                );


                updateStatus(
                        scanId,
                        "API_ERROR"
                );

                return;
            }


            GoogleGeolocationResponse result =
                    jsonMapper.readValue(

                            response.body(),

                            GoogleGeolocationResponse.class
                    );


            if (
                    result.location() == null ||
                    result.location().lat() == null ||
                    result.location().lng() == null
            ) {

                updateStatus(
                        scanId,
                        "NOT_FOUND"
                );

                return;
            }


            double accuracy =
                    result.accuracy() != null
                            ? result.accuracy()
                            : 0.0;


            updateLocation(

                    scanId,

                    result.location().lat(),

                    result.location().lng(),

                    accuracy
            );


            System.out.println(
                    "[GEOLOCATION] LOCATED"
                    + " | scanId="
                    + scanId
                    + " | lat="
                    + result.location().lat()
                    + " | lng="
                    + result.location().lng()
                    + " | accuracy="
                    + accuracy
                    + "m"
            );

        }
        catch (InterruptedException e) {

            Thread.currentThread()
                    .interrupt();


            updateStatus(
                    scanId,
                    "ERROR"
            );

        }
        catch (Exception e) {

            System.err.println(
                    "[GEOLOCATION] Error: "
                            + e.getMessage()
            );


            updateStatus(
                    scanId,
                    "ERROR"
            );
        }
    }


    private boolean isUsableBssid(
            String bssid
    ) {

        if (
                bssid == null ||
                !bssid.matches(
                        "(?i)^([0-9a-f]{2}:){5}[0-9a-f]{2}$"
                )
        ) {

            return false;
        }


        int firstOctet =
                Integer.parseInt(

                        bssid.substring(
                                0,
                                2
                        ),

                        16
                );


        /*
         * Bit U/L = 1:
         * locally administered MAC.
         *
         * Google bo qua nhung MAC nay.
         */
        return (
                firstOctet & 0x02
        ) == 0;
    }


    private void updateLocation(

            Long scanId,

            double latitude,

            double longitude,

            double accuracyMeters

    ) {

        locationScanRepository
                .findById(
                        scanId
                )
                .ifPresent(
                        scan -> {

                            scan.markLocated(

                                    latitude,

                                    longitude,

                                    accuracyMeters
                            );


                            locationScanRepository
                                    .save(
                                            scan
                                    );
                        }
                );
    }


    private void updateStatus(

            Long scanId,

            String status

    ) {

        locationScanRepository
                .findById(
                        scanId
                )
                .ifPresent(
                        scan -> {

                            scan.markLocationStatus(
                                    status
                            );


                            locationScanRepository
                                    .save(
                                            scan
                                    );
                        }
                );
    }
}