package com.smartpackage.backend.mqtt;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.RejectedExecutionException;

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
import com.smartpackage.backend.ai.MotionService;

@Component
public class MqttSubscriber implements MqttCallbackExtended {

    private static final Logger logger = LoggerFactory.getLogger(MqttSubscriber.class);
    private final MqttMessageService mqttMessageService;
    private final MotionService motionService;
    private final String broker;
    private final String clientId;
    private final String username;
    private final String password;
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
    private final ThreadPoolExecutor receiptExecutor = new ThreadPoolExecutor(
            1, 1, 0, TimeUnit.MILLISECONDS, new ArrayBlockingQueue<>(128), task -> {
                Thread thread = new Thread(task, "mqtt-receipts");
                thread.setDaemon(true);
                return thread;
            });

    public MqttSubscriber(
            MqttMessageService mqttMessageService,
            MotionService motionService,
            @Value("${app.mqtt.broker}") String broker,
            @Value("${app.mqtt.client-id}") String clientId,
            @Value("${app.mqtt.topic.telemetry}") String telemetryTopic,
            @Value("${app.mqtt.topic.event}") String eventTopic,
            @Value("${app.mqtt.topic.location}") String locationTopic,
            @Value("${app.mqtt.retry-interval-ms:5000}") long retryIntervalMillis,
            @Value("${app.mqtt.username:}") String username,
            @Value("${app.mqtt.password:}") String password) {
        if (retryIntervalMillis <= 0) {
            throw new IllegalArgumentException("MQTT retry interval must be positive");
        }
        if (username.isBlank() && !password.isEmpty()) {
            throw new IllegalArgumentException("MQTT username is required when a password is configured");
        }
        this.mqttMessageService = mqttMessageService;
        this.motionService = motionService;
        this.broker = broker;
        this.clientId = clientId;
        this.username = username;
        this.password = password;
        this.topics = new String[] {telemetryTopic, eventTopic, locationTopic, "smart-package/+/motion-window"};
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
                if (!username.isBlank()) {
                    options.setUserName(username);
                    options.setPassword(password.toCharArray());
                }
                logger.info("[MQTT] Connecting to: {}", broker);
                client.connect(options);
            }
            if (!stopping && !subscribed) {
                // Keep SUBACK waits off the Paho callback thread.
                client.subscribe(topics, new int[] {0, 1, 0, 1});
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
        if (topic.endsWith("/motion-window")) {
            try {
                String receipt=motionService.accept(topic,new String(message.getPayload(),StandardCharsets.UTF_8));
                if(receipt!=null && !stopping) receiptExecutor.execute(()-> {
                    try {client.publish(topic.replace("/motion-window","/motion-ack"),receipt.getBytes(StandardCharsets.UTF_8),1,false);}
                    catch(MqttException e) {logger.warn("Motion receipt failed; device will retry");}
                });
            } catch(Exception e) {logger.warn("Motion window rejected: {}",e.getMessage());}
            return;
        }
        var receipt = mqttMessageService.handle(
                topic, new String(message.getPayload(), StandardCharsets.UTF_8));
        if (receipt != null && !stopping) {
            try {
                receiptExecutor.execute(() -> publishReceipt(receipt));
            } catch (RejectedExecutionException exception) {
                logger.warn("Receipt queue unavailable; device will retry event {}", receipt.eventId());
            }
        }
    }

    private void publishReceipt(MqttMessageService.EventReceipt receipt) {
        try {
            String payload = "{\"deviceId\":\"" + receipt.deviceId()
                    + "\",\"eventId\":" + receipt.eventId() + ",\"status\":\"SAVED\"}";
            client.publish("smart-package/" + receipt.deviceId() + "/event-ack",
                    payload.getBytes(StandardCharsets.UTF_8), 1, false);
        } catch (MqttException exception) {
            logger.warn("Receipt publish failed for {} / {}; device will retry",
                    receipt.deviceId(), receipt.eventId());
        }
    }

    public void publishMotionControl(String deviceId,String payload) throws MqttException {
        if(!isReady()) throw new MqttException(MqttException.REASON_CODE_CLIENT_NOT_CONNECTED);
        if(!deviceId.matches("[A-Za-z0-9_-]{1,64}")) throw new IllegalArgumentException("Mã thiết bị không hợp lệ.");
        client.publish("smart-package/"+deviceId+"/motion-control",payload.getBytes(StandardCharsets.UTF_8),1,false);
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // Receipt delivery to the broker needs no additional application action.
    }

    @PreDestroy
    public void stop() {
        stopping = true;
        subscribed = false;
        connectionExecutor.shutdownNow();
        receiptExecutor.shutdownNow();
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
