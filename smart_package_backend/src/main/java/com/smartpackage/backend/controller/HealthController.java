package com.smartpackage.backend.controller;

import java.time.LocalDateTime;
import java.util.LinkedHashMap;
import java.util.Map;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;


@RestController
@RequestMapping("/api")
public class HealthController {


    @GetMapping("/health")
    public Map<String, Object> health() {

        Map<String, Object> response =
                new LinkedHashMap<>();


        response.put(
                "status",
                "OK"
        );


        response.put(
                "service",
                "smart-package-backend"
        );


        response.put(
                "time",
                LocalDateTime.now()
        );


        return response;
    }
}