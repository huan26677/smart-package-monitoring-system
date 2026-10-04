package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.PackageEventEntity;


public interface PackageEventRepository
        extends JpaRepository<
                PackageEventEntity,
                Long
        > {


    boolean existsByDevice_DeviceIdAndEventId(
            String deviceId,
            long eventId
    );
}