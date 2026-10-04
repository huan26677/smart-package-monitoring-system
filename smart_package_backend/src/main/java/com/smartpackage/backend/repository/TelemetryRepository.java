package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.TelemetryEntity;


public interface TelemetryRepository
        extends JpaRepository<
                TelemetryEntity,
                Long
        > {
}