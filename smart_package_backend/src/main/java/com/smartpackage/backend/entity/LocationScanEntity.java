package com.smartpackage.backend.entity;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

import jakarta.persistence.CascadeType;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.OneToMany;
import jakarta.persistence.PrePersist;
import jakarta.persistence.Table;


@Entity
@Table(name = "location_scans")
public class LocationScanEntity {


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
            name = "scan_timestamp",
            nullable = false
    )
    private long scanTimestamp;


    @Column(
            name = "received_at",
            nullable = false
    )
    private Instant receivedAt;


    @OneToMany(
            mappedBy = "locationScan",
            cascade = CascadeType.ALL,
            orphanRemoval = true
    )
    private List<WifiAccessPointEntity>
            wifiAccessPoints =
                    new ArrayList<>();


    protected LocationScanEntity() {
    }


    public LocationScanEntity(
            DeviceEntity device,
            long scanTimestamp
    ) {

        this.device =
                device;

        this.scanTimestamp =
                scanTimestamp;
    }


    @PrePersist
    private void prePersist() {

        if (receivedAt == null) {

            receivedAt =
                    Instant.now();
        }
    }


    public void addWifiAccessPoint(
            WifiAccessPointEntity accessPoint
    ) {

        wifiAccessPoints.add(
                accessPoint
        );

        accessPoint.setLocationScan(
                this
        );
    }


    public Long getId() {

        return id;
    }


    public DeviceEntity getDevice() {

        return device;
    }


    public long getScanTimestamp() {

        return scanTimestamp;
    }


    public Instant getReceivedAt() {

        return receivedAt;
    }


    public List<WifiAccessPointEntity>
            getWifiAccessPoints() {

        return wifiAccessPoints;
    }
}