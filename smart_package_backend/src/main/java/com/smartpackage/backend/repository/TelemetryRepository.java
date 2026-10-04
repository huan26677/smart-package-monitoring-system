package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.TelemetryEntity;


public interface TelemetryRepository
        extends JpaRepository<
                TelemetryEntity,
                Long
        > {


    java.util.Optional<TelemetryEntity>
            findFirstByDevice_DeviceIdOrderByIdDesc(
                    String deviceId
            );


    java.util.List<TelemetryEntity>
            findByDevice_DeviceIdOrderByIdDesc(
                    String deviceId,
                    org.springframework.data.domain.Pageable pageable
            );
}