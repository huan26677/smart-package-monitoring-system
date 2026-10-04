package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.DeviceEntity;


public interface DeviceRepository
        extends JpaRepository<
                DeviceEntity,
                String
        > {
}