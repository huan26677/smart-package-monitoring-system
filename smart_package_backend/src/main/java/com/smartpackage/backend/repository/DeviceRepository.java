package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.DeviceEntity;


public interface DeviceRepository
        extends JpaRepository<
                DeviceEntity,
                String
        > {

    @org.springframework.data.jpa.repository.Lock(jakarta.persistence.LockModeType.PESSIMISTIC_WRITE)
    @org.springframework.data.jpa.repository.Query("select d from DeviceEntity d where d.deviceId=:deviceId")
    java.util.Optional<DeviceEntity> findForCapture(String deviceId);


    java.util.List<DeviceEntity>
            findAllByOrderByDeviceIdAsc();
}
