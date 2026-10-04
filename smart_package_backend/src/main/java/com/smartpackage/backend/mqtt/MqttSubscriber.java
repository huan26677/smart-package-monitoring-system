package com.smartpackage.backend.mqtt;
import java.nio.charset.StandardCharsets;
import jakarta.annotation.PreDestroy;

import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import com.smartpackage.backend.service.MqttMessageService;

@Component
public class MqttSubscriber
        implements MqttCallbackExtended {


    private final MqttMessageService
        mqttMessageService;

    private final String broker;

    private final String clientId;

    private final String telemetryTopic;

    private final String eventTopic;

    private final String locationTopic;


    private MqttClient client;


    public MqttSubscriber(

            MqttMessageService
                mqttMessageService,

            @Value("${app.mqtt.broker}")
            String broker,

            @Value("${app.mqtt.client-id}")
            String clientId,

            @Value("${app.mqtt.topic.telemetry}")
            String telemetryTopic,

            @Value("${app.mqtt.topic.event}")
            String eventTopic,

            @Value("${app.mqtt.topic.location}")
            String locationTopic

    ) {

        this.mqttMessageService =
                mqttMessageService;

        this.broker =
                broker;

        this.clientId =
                clientId;

        this.telemetryTopic =
                telemetryTopic;

        this.eventTopic =
                eventTopic;

        this.locationTopic =
                locationTopic;


        start();
    }


    /* =====================================================
     * START
     * ===================================================== */

    private void start() {

        try {

            System.out.println(
                    "[MQTT] Connecting to: "
                            + broker
            );


            client =
                    new MqttClient(
                            broker,
                            clientId
                    );


            client.setCallback(
                    this
            );


            MqttConnectOptions options =
                    new MqttConnectOptions();


            options.setAutomaticReconnect(
                    true
            );


            options.setCleanSession(
                    true
            );


            options.setConnectionTimeout(
                    10
            );


            options.setKeepAliveInterval(
                    30
            );


            client.connect(
                    options
            );


        } catch (Exception e) {

            System.err.println(
                    "[MQTT] Connection failed: "
                            + e.getMessage()
            );

        }
    }


    /* =====================================================
     * CONNECT COMPLETE
     * ===================================================== */

    @Override
    public void connectComplete(
            boolean reconnect,
            String serverURI
    ) {

        System.out.println(
                "[MQTT] CONNECTED: "
                        + serverURI
        );


        try {

            /*
             * Subscribe lai sau moi lan reconnect.
             */
            client.subscribe(
                    telemetryTopic,
                    0
            );


            client.subscribe(
                    eventTopic,
                    1
            );


            client.subscribe(
                    locationTopic,
                    0
            );


            System.out.println(
                    "[MQTT] SUBSCRIBED:"
            );


            System.out.println(
                    "  "
                            + telemetryTopic
            );


            System.out.println(
                    "  "
                            + eventTopic
            );


            System.out.println(
                    "  "
                            + locationTopic
            );


        } catch (Exception e) {

            System.err.println(
                    "[MQTT] Subscribe error: "
                            + e.getMessage()
            );

        }
    }


    /* =====================================================
     * CONNECTION LOST
     * ===================================================== */

    @Override
    public void connectionLost(
            Throwable cause
    ) {

        System.err.println(
                "[MQTT] CONNECTION LOST"
        );


        if (cause != null) {

            System.err.println(
                    "[MQTT] "
                            + cause.getMessage()
            );
        }
    }


    /* =====================================================
     * MESSAGE ARRIVED
     * ===================================================== */

    @Override
    public void messageArrived(
            String topic,
            MqttMessage message
    ) {

        String payload =
                new String(
                        message.getPayload(),
                        StandardCharsets.UTF_8
                );

        System.out.println();


        System.out.println(
                "========================================"
        );


        System.out.println(
                "[MQTT] MESSAGE RECEIVED"
        );


        System.out.println(
                "Topic   : "
                        + topic
        );


        System.out.println(
                "QoS     : "
                        + message.getQos()
        );


        System.out.println(
                "Payload : "
                        + payload
        );

        mqttMessageService.handle(
                topic,
                payload
        );

        System.out.println(
                "========================================"
        );

    }


    /* =====================================================
     * DELIVERY COMPLETE
     * ===================================================== */

    @Override
    public void deliveryComplete(
            org.eclipse.paho.client.mqttv3.IMqttDeliveryToken token
    ) {

        /*
         * Backend hien chi subscribe,
         * chua publish MQTT.
         */
    }


    /* =====================================================
     * SHUTDOWN
     * ===================================================== */

    @PreDestroy
    public void stop() {

        try {

            if (
                    client != null &&
                    client.isConnected()
            ) {

                client.disconnect();

            }


            if (client != null) {

                client.close();

            }


        } catch (Exception e) {

            System.err.println(
                    "[MQTT] Shutdown error: "
                            + e.getMessage()
            );
        }
    }
}