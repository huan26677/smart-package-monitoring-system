package com.smartpackage.backend.entity;

import java.time.Instant;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.PrePersist;
import jakarta.persistence.Table;


@Entity
@Table(name = "telemetry")
public class TelemetryEntity {


    @Id
    @GeneratedValue(
            strategy = GenerationType.IDENTITY
    )
    private Long id;


    @ManyToOne(
            fetch = FetchType.LAZY,
            optional = false
    )
    @JoinColumn(
            name = "device_id",
            nullable = false
    )
    private DeviceEntity device;


    @Column(
            name = "g_force",
            nullable = false
    )
    private double gForce;


    @Column(
            name = "angle",
            nullable = false
    )
    private double angle;


    @Column(
            name = "vibration",
            nullable = false
    )
    private double vibration;


    @Column(
            name = "state",
            length = 32,
            nullable = false
    )
    private String state;


    @Column(
            name = "wifi_rssi",
            nullable = false
    )
    private int wifiRssi;


    @Column(
            name = "received_at",
            nullable = false
    )
    private Instant receivedAt;


    @Column(name="pending_events")
    private Integer pendingEvents;
    @Column(name="rejected_events")
    private Long rejectedEvents;
    public void setQueueStatus(Integer pendingEvents, Long rejectedEvents) {
        this.pendingEvents = pendingEvents; this.rejectedEvents = rejectedEvents;
    }
    public Integer getPendingEvents() { return pendingEvents; }
    public Long getRejectedEvents() { return rejectedEvents; }

    protected TelemetryEntity() {
    }


    public TelemetryEntity(
            DeviceEntity device,
            double gForce,
            double angle,
            double vibration,
            String state,
            int wifiRssi
    ) {

        this.device =
                device;

        this.gForce =
                gForce;

        this.angle =
                angle;

        this.vibration =
                vibration;

        this.state =
                state;

        this.wifiRssi =
                wifiRssi;
    }


    @PrePersist
    private void prePersist() {

        if (receivedAt == null) {

            receivedAt =
                    Instant.now();
        }
    }


    public Long getId() {

        return id;
    }


    public DeviceEntity getDevice() {

        return device;
    }


    public double getGForce() {

        return gForce;
    }


    public double getAngle() {

        return angle;
    }


    public double getVibration() {

        return vibration;
    }


    public String getState() {

        return state;
    }


    public int getWifiRssi() {

        return wifiRssi;
    }


    public Instant getReceivedAt() {

        return receivedAt;
    }
}