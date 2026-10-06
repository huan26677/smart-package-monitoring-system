package com.smartpackage.backend.ai;
import java.util.List;
public record MotionWindow(String deviceId, int schemaVersion, String bootId, long windowId,
        String sessionId, int sessionWindow, long timestamp, String ruleLabel, List<List<Double>> samples) {}
