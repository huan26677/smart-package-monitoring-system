package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.LocationScanEntity;


public interface LocationScanRepository
        extends JpaRepository<
                LocationScanEntity,
                Long
        > {
}