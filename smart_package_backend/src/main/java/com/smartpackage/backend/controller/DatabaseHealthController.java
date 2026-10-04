package com.smartpackage.backend.controller;

import java.util.LinkedHashMap;
import java.util.Map;

import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;


@RestController
@RequestMapping("/api/health")
public class DatabaseHealthController {


    private final JdbcTemplate jdbcTemplate;


    public DatabaseHealthController(
            JdbcTemplate jdbcTemplate
    ) {

        this.jdbcTemplate =
                jdbcTemplate;
    }


    @GetMapping("/database")
    public Map<String, Object>
            databaseHealth() {


        Map<String, Object> response =
                new LinkedHashMap<>();


        Integer result =
                jdbcTemplate.queryForObject(
                        "SELECT 1",
                        Integer.class
                );


        response.put(
                "status",
                result != null &&
                result == 1
                        ? "OK"
                        : "ERROR"
        );


        response.put(
                "database",
                "PostgreSQL"
        );


        response.put(
                "databaseName",
                "smart_package"
        );


        return response;
    }
}