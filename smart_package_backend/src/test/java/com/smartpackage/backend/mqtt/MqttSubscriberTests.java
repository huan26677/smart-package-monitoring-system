package com.smartpackage.backend.mqtt;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.junit.jupiter.api.Test;
import org.mockito.MockedConstruction;

import com.smartpackage.backend.service.MqttMessageService;

class MqttSubscriberTests {

    @Test
    void retriesWhenBrokerIsUnavailableAtStartup() throws Exception {
        AtomicBoolean connected = new AtomicBoolean();
        AtomicInteger attempts = new AtomicInteger();
        try (MockedConstruction<MqttClient> clients = mockConstruction(MqttClient.class,
                (client, context) -> {
                    when(client.isConnected()).thenAnswer(call -> connected.get());
                    doAnswer(call -> {
                        MqttConnectOptions options = call.getArgument(0);
                        assertFalse(options.isAutomaticReconnect());
                        if (attempts.incrementAndGet() == 1) {
                            throw new MqttException(MqttException.REASON_CODE_SERVER_CONNECT_ERROR);
                        }
                        connected.set(true);
                        return null;
                    }).when(client).connect(any(MqttConnectOptions.class));
                })) {
            MqttSubscriber subscriber = subscriber();
            try {
                subscriber.start();
                await(subscriber::isReady);
                assertTrue(attempts.get() >= 2);
                verify(clients.constructed().get(0)).subscribe(
                        new String[] {"test/+/telemetry", "test/+/event", "test/+/location-scan"},
                        new int[] {0, 1, 0});
            } finally {
                subscriber.stop();
            }
        }
    }

    @Test
    void reconnectsAndResubscribesAfterConnectionLoss() throws Exception {
        AtomicBoolean connected = new AtomicBoolean();
        AtomicInteger attempts = new AtomicInteger();
        try (MockedConstruction<MqttClient> clients = mockConstruction(MqttClient.class,
                (client, context) -> {
                    when(client.isConnected()).thenAnswer(call -> connected.get());
                    doAnswer(call -> {
                        attempts.incrementAndGet();
                        connected.set(true);
                        return null;
                    }).when(client).connect(any(MqttConnectOptions.class));
                })) {
            MqttSubscriber subscriber = subscriber();
            try {
                subscriber.start();
                await(subscriber::isReady);
                connected.set(false);
                assertFalse(subscriber.isReady());
                subscriber.connectionLost(new IllegalStateException("broker restarted"));
                await(() -> attempts.get() >= 2 && subscriber.isReady());
                verify(clients.constructed().get(0), times(2)).subscribe(
                        any(String[].class), any(int[].class));
            } finally {
                subscriber.stop();
            }
            assertFalse(subscriber.isReady());
            verify(clients.constructed().get(0)).disconnectForcibly(anyLong(), anyLong(), anyBoolean());
            verify(clients.constructed().get(0)).close(true);
        }
    }

    @Test
    void retriesSubscriptionsWithoutReconnectingHealthyClient() throws Exception {
        AtomicInteger subscriptions = new AtomicInteger();
        try (MockedConstruction<MqttClient> clients = mockConstruction(MqttClient.class,
                (client, context) -> {
                    when(client.isConnected()).thenReturn(true);
                    doAnswer(call -> {
                        if (subscriptions.incrementAndGet() == 1) {
                            throw new MqttException(MqttException.REASON_CODE_CLIENT_TIMEOUT);
                        }
                        return null;
                    }).when(client).subscribe(any(String[].class), any(int[].class));
                })) {
            MqttSubscriber subscriber = subscriber();
            try {
                subscriber.start();
                await(subscriber::isReady);
                assertTrue(subscriptions.get() >= 2);
                verify(clients.constructed().get(0), times(0)).connect(any(MqttConnectOptions.class));
            } finally {
                subscriber.stop();
            }
        }
    }

    private MqttSubscriber subscriber() {
        return new MqttSubscriber(mock(MqttMessageService.class), "tcp://localhost:1883",
                "test-client", "test/+/telemetry", "test/+/event", "test/+/location-scan", 25);
    }

    private void await(BooleanSupplier condition) throws InterruptedException {
        long deadline = System.nanoTime() + java.util.concurrent.TimeUnit.SECONDS.toNanos(3);
        while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
            Thread.sleep(10);
        }
        assertTrue(condition.getAsBoolean(), "MQTT did not recover before timeout");
    }
}
