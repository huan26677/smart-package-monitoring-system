package com.smartpackage.backend.entity;

import java.time.Instant;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;


@Entity
@Table(name = "devices")
public class DeviceEntity {


    @Id
    @Column(
            name = "device_id",
            length = 64,
            nullable = false
    )
    private String deviceId;


    @Column(
            name = "created_at",
            nullable = false
    )
    private Instant createdAt;


    @Column(
            name = "last_seen_at",
            nullable = false
    )
    private Instant lastSeenAt;


    protected DeviceEntity() {
    }


    public DeviceEntity(
            String deviceId
    ) {

        this.deviceId =
                deviceId;

        Instant now =
                Instant.now();

        this.createdAt =
                now;

        this.lastSeenAt =
                now;
    }


    public void touch() {

        this.lastSeenAt =
                Instant.now();
    }


    public String getDeviceId() {

        return deviceId;
    }


    public Instant getCreatedAt() {

        return createdAt;
    }


    public Instant getLastSeenAt() {

        return lastSeenAt;
    }
}