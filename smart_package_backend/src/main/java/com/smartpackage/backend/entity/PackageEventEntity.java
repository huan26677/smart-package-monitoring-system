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
import jakarta.persistence.UniqueConstraint;


@Entity
@Table(
        name = "package_events",
        uniqueConstraints = {
                @UniqueConstraint(
                        name =
                                "uk_package_event_device_event",
                        columnNames = {
                                "device_id",
                                "event_id"
                        }
                )
        }
)
public class PackageEventEntity {


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
            name = "event_id",
            nullable = false
    )
    private long eventId;


    @Column(
            name = "event_type",
            length = 32,
            nullable = false
    )
    private String type;


    @Column(
            name = "impact_level",
            length = 32
    )
    private String level;


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
            name = "uptime_ms",
            nullable = false
    )
    private long uptimeMs;


    @Column(
            name = "event_timestamp",
            nullable = false
    )
    private long eventTimestamp;


    @Column(
            name = "time_text",
            length = 32
    )
    private String timeText;


    @Column(
            name = "received_at",
            nullable = false
    )
    private Instant receivedAt;


    protected PackageEventEntity() {
    }


    public PackageEventEntity(
            DeviceEntity device,
            long eventId,
            String type,
            String level,
            double gForce,
            double angle,
            double vibration,
            long uptimeMs,
            long eventTimestamp,
            String timeText
    ) {

        this.device =
                device;

        this.eventId =
                eventId;

        this.type =
                type;

        this.level =
                level;

        this.gForce =
                gForce;

        this.angle =
                angle;

        this.vibration =
                vibration;

        this.uptimeMs =
                uptimeMs;

        this.eventTimestamp =
                eventTimestamp;

        this.timeText =
                timeText;
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


    public long getEventId() {

        return eventId;
    }


    public String getType() {

        return type;
    }


    public String getLevel() {

        return level;
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


    public long getUptimeMs() {

        return uptimeMs;
    }


    public long getEventTimestamp() {

        return eventTimestamp;
    }


    public String getTimeText() {

        return timeText;
    }


    public Instant getReceivedAt() {

        return receivedAt;
    }
}