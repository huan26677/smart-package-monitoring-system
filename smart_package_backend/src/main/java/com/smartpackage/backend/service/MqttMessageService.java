package com.smartpackage.backend.service;

import org.springframework.stereotype.Service;

import com.smartpackage.backend.dto.LocationScanMessage;
import com.smartpackage.backend.dto.PackageEventMessage;
import com.smartpackage.backend.dto.TelemetryMessage;

import tools.jackson.databind.json.JsonMapper;


@Service
public class MqttMessageService {


    private final JsonMapper jsonMapper;

    private final MqttPersistenceService
        mqttPersistenceService;

    private final WifiGeolocationService
        wifiGeolocationService;

    public MqttMessageService(

        JsonMapper jsonMapper,

        MqttPersistenceService
                mqttPersistenceService,
        WifiGeolocationService
                wifiGeolocationService

     ) {

        this.jsonMapper =
                jsonMapper;

        this.mqttPersistenceService =
                mqttPersistenceService;
        this.wifiGeolocationService =
                wifiGeolocationService;
     }


    /* =====================================================
     * HANDLE MQTT MESSAGE
     * ===================================================== */

    public void handle(
            String topic,
            String payload
    ) {

        try {

            if (
                    topic.endsWith(
                            "/telemetry"
                    )
            ) {

                handleTelemetry(
                        payload
                );

            }

            else if (
                    topic.endsWith(
                            "/event"
                    )
            ) {

                handleEvent(
                        payload
                );

            }

            else if (
                    topic.endsWith(
                            "/location-scan"
                    )
            ) {

                handleLocationScan(
                        payload
                );

            }

            else {

                System.out.println(
                        "[MQTT] Unknown topic: "
                                + topic
                );

            }

        }
        catch (Exception e) {

            System.err.println(
                    "[MQTT] JSON parse error"
            );


            System.err.println(
                    "[MQTT] Topic: "
                            + topic
            );


            System.err.println(
                    "[MQTT] Error: "
                            + e.getMessage()
            );

        }
    }


    /* =====================================================
     * TELEMETRY
     * ===================================================== */

    private void handleTelemetry(
            String payload
    ) throws Exception {

        TelemetryMessage data =
                jsonMapper.readValue(
                        payload,
                        TelemetryMessage.class
                );

        mqttPersistenceService
                .saveTelemetry(
                        data
                );

        System.out.println(
                "[PARSED TELEMETRY]"
        );


        System.out.println(
                "Device    : "
                        + data.deviceId()
        );


        System.out.println(
                "G         : "
                        + data.g()
        );


        System.out.println(
                "Angle     : "
                        + data.angle()
        );


        System.out.println(
                "Vibration : "
                        + data.vibration()
        );


        System.out.println(
                "State     : "
                        + data.state()
        );


        System.out.println(
                "WiFi RSSI : "
                        + data.rssi()
                        + " dBm"
        );
    }


    /* =====================================================
     * EVENT
     * ===================================================== */

    private void handleEvent(
            String payload
    ) throws Exception {

        PackageEventMessage data =
                jsonMapper.readValue(
                        payload,
                        PackageEventMessage.class
                );

        boolean eventSaved =
                mqttPersistenceService
                        .saveEvent(
                                data
                        );

        System.out.println(
                "[PARSED EVENT]"
        );

        System.out.println(
                "Database  : "
                        + (
                                eventSaved
                                        ? "SAVED"
                                        : "DUPLICATE - SKIPPED"
                        )
        );

        System.out.println(
                "Device    : "
                        + data.deviceId()
        );


        System.out.println(
                "Event ID  : "
                        + data.eventId()
        );


        System.out.println(
                "Type      : "
                        + data.type()
        );


        System.out.println(
                "Level     : "
                        + data.level()
        );


        System.out.println(
                "G         : "
                        + data.g()
        );


        System.out.println(
                "Angle     : "
                        + data.angle()
        );


        System.out.println(
                "Vibration : "
                        + data.vibration()
        );


        System.out.println(
                "Timestamp : "
                        + data.timestamp()
        );


        System.out.println(
                "Time      : "
                        + data.timeText()
        );
    }


    /* =====================================================
     * LOCATION SCAN
     * ===================================================== */

    private void handleLocationScan(
            String payload
    ) throws Exception {

        LocationScanMessage data =
                jsonMapper.readValue(
                        payload,
                        LocationScanMessage.class
                );

        Long locationScanId =
                mqttPersistenceService
                        .saveLocationScan(
                                data
                        );


        wifiGeolocationService
                .resolveAndStore(

                        locationScanId,

                        data.wifiAccessPoints()
                );

        int accessPointCount = 0;


        if (
                data.wifiAccessPoints()
                        != null
        ) {

            accessPointCount =
                    data
                            .wifiAccessPoints()
                            .size();

        }


        System.out.println(
                "[PARSED LOCATION SCAN]"
        );


        System.out.println(
                "Device    : "
                        + data.deviceId()
        );


        System.out.println(
                "Timestamp : "
                        + data.timestamp()
        );


        System.out.println(
                "WiFi APs  : "
                        + accessPointCount
        );


        if (
                data.wifiAccessPoints()
                        != null
        ) {

            data
                    .wifiAccessPoints()
                    .forEach(
                            ap ->
                                    System.out.println(
                                            "  "
                                            + ap.bssid()
                                            + " | "
                                            + ap.rssi()
                                            + " dBm"
                                    )
                    );

        }
    }
}