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

        java.util.List<PackageEventEntity>
                findByDevice_DeviceIdOrderByIdDesc(
                        String deviceId,
                        org.springframework.data.domain.Pageable pageable
                );
}