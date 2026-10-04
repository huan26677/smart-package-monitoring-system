package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.LocationScanEntity;


public interface LocationScanRepository
        extends JpaRepository<
                LocationScanEntity,
                Long
        > {


    java.util.List<LocationScanEntity>
            findByDevice_DeviceIdOrderByIdDesc(
                    String deviceId,
                    org.springframework.data.domain.Pageable pageable
            );
    java.util.Optional<LocationScanEntity>
            findFirstByDevice_DeviceIdAndLatitudeIsNotNullAndLongitudeIsNotNullOrderByIdDesc(
                    String deviceId
            );
}