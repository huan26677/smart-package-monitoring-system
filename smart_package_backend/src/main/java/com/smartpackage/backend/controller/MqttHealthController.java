package com.smartpackage.backend.controller;

import java.util.Map;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import com.smartpackage.backend.mqtt.MqttSubscriber;

@RestController
public class MqttHealthController {

    private final MqttSubscriber mqttSubscriber;

    public MqttHealthController(MqttSubscriber mqttSubscriber) {
        this.mqttSubscriber = mqttSubscriber;
    }

    @GetMapping("/api/health/mqtt")
    public ResponseEntity<Map<String, Object>> health() {
        boolean ready = mqttSubscriber.isReady();
        return ResponseEntity.status(ready ? 200 : 503)
                .body(Map.of("status", ready ? "OK" : "UNAVAILABLE", "subscribed", ready));
    }
}
