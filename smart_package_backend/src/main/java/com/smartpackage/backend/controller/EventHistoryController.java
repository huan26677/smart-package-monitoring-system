package com.smartpackage.backend.controller;

import java.time.Instant;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import com.smartpackage.backend.service.EventHistoryService;

@RestController
@RequestMapping("/api/devices/{deviceId}")
public class EventHistoryController {
    private final EventHistoryService service;
    public EventHistoryController(EventHistoryService service) { this.service = service; }
    private Instant instant(String value) {
        if (value == null || value.isBlank()) return null;
        try { return Instant.parse(value); }
        catch (java.time.DateTimeException exception) { throw new IllegalArgumentException("Invalid ISO timestamp"); }
    }
    @GetMapping("/event-history")
    public EventHistoryService.History history(@PathVariable String deviceId,
            @RequestParam(required = false) String type, @RequestParam(required = false) String level,
            @RequestParam(required = false) String from, @RequestParam(required = false) String to,
            @RequestParam(defaultValue = "0") int page, @RequestParam(defaultValue = "20") int size) {
        return service.history(deviceId, type, level, instant(from), instant(to), page, size);
    }
    @GetMapping("/event-summary")
    public EventHistoryService.Summary summary(@PathVariable String deviceId,
            @RequestParam(required = false) String type, @RequestParam(required = false) String level,
            @RequestParam(required = false) String from, @RequestParam(required = false) String to) {
        return service.summary(deviceId, type, level, instant(from), instant(to));
    }
    @GetMapping(value = "/events.csv", produces = "text/csv;charset=UTF-8")
    public ResponseEntity<String> csv(@PathVariable String deviceId,
            @RequestParam(required = false) String type, @RequestParam(required = false) String level,
            @RequestParam(required = false) String from, @RequestParam(required = false) String to) {
        String name = "events-" + deviceId.replaceAll("[^A-Za-z0-9_-]", "_") + ".csv";
        return ResponseEntity.ok().header("Content-Disposition", "attachment; filename=\"" + name + "\"")
                .body(service.csv(deviceId, type, level, instant(from), instant(to)));
    }
}
