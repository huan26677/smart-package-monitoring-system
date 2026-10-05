package com.smartpackage.backend.mqtt;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import com.smartpackage.backend.service.MqttMessageService;

@Component
public class MqttSubscriber implements MqttCallbackExtended {

    private static final Logger logger = LoggerFactory.getLogger(MqttSubscriber.class);
    private final MqttMessageService mqttMessageService;
    private final String broker;
    private final String clientId;
    private final String[] topics;
    private final long retryIntervalMillis;
    private final ScheduledExecutorService connectionExecutor =
            Executors.newSingleThreadScheduledExecutor(task -> {
                Thread thread = new Thread(task, "mqtt-connection");
                thread.setDaemon(true);
                return thread;
            });

    private volatile MqttClient client;
    private volatile boolean subscribed;
    private volatile boolean stopping;

    public MqttSubscriber(
            MqttMessageService mqttMessageService,
            @Value("${app.mqtt.broker}") String broker,
            @Value("${app.mqtt.client-id}") String clientId,
            @Value("${app.mqtt.topic.telemetry}") String telemetryTopic,
            @Value("${app.mqtt.topic.event}") String eventTopic,
            @Value("${app.mqtt.topic.location}") String locationTopic,
            @Value("${app.mqtt.retry-interval-ms:5000}") long retryIntervalMillis) {
        if (retryIntervalMillis <= 0) {
            throw new IllegalArgumentException("MQTT retry interval must be positive");
        }
        this.mqttMessageService = mqttMessageService;
        this.broker = broker;
        this.clientId = clientId;
        this.topics = new String[] {telemetryTopic, eventTopic, locationTopic};
        this.retryIntervalMillis = retryIntervalMillis;
    }

    @PostConstruct
    void start() throws MqttException {
        client = new MqttClient(broker, clientId, new MemoryPersistence());
        client.setCallback(this);
        client.setTimeToWait(10000);
        // One worker owns all retries, including the first connection at startup.
        connectionExecutor.scheduleWithFixedDelay(
                this::connectAndSubscribe, 0, retryIntervalMillis, TimeUnit.MILLISECONDS);
    }

    private synchronized void connectAndSubscribe() {
        if (stopping) {
            return;
        }
        try {
            if (!client.isConnected()) {
                subscribed = false;
                MqttConnectOptions options = new MqttConnectOptions();
                options.setAutomaticReconnect(false);
                options.setCleanSession(true);
                options.setConnectionTimeout(10);
                options.setKeepAliveInterval(30);
                logger.info("[MQTT] Connecting to: {}", broker);
                client.connect(options);
            }
            if (!stopping && !subscribed) {
                // Keep SUBACK waits off the Paho callback thread.
                client.subscribe(topics, new int[] {0, 1, 0});
                subscribed = true;
                logger.info("[MQTT] SUBSCRIBED: {}", String.join(", ", topics));
            }
        } catch (Exception exception) {
            subscribed = false;
            if (!stopping) {
                logger.warn("[MQTT] Connection/subscription failed; retrying in {} ms: {}",
                        retryIntervalMillis, exception.getMessage());
            }
        }
    }

    public boolean isReady() {
        MqttClient currentClient = client;
        return !stopping && subscribed && currentClient != null && currentClient.isConnected();
    }

    @Override
    public void connectComplete(boolean reconnect, String serverURI) {
        logger.info("[MQTT] CONNECTED: {}", serverURI);
    }

    @Override
    public void connectionLost(Throwable cause) {
        subscribed = false;
        if (!stopping) {
            logger.warn("[MQTT] Connection lost; reconnecting in up to {} ms: {}",
                    retryIntervalMillis, cause == null ? "unknown cause" : cause.getMessage());
        }
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) {
        mqttMessageService.handle(topic, new String(message.getPayload(), StandardCharsets.UTF_8));
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // This client only subscribes.
    }

    @PreDestroy
    public void stop() {
        stopping = true;
        subscribed = false;
        connectionExecutor.shutdownNow();
        // Wait for the bounded connection attempt before closing the client.
        synchronized (this) {
            if (client != null) {
                try {
                    client.disconnectForcibly(0, 1000, false);
                } catch (MqttException exception) {
                    logger.debug("[MQTT] Disconnect during shutdown: {}", exception.getMessage());
                } finally {
                    try {
                        client.close(true);
                    } catch (MqttException exception) {
                        logger.warn("[MQTT] Close failed: {}", exception.getMessage());
                    }
                }
            }
        }
    }
}
