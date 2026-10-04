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

        @Column(
                name = "latitude"
        )
        private Double latitude;


        @Column(
                name = "longitude"
        )
        private Double longitude;


        @Column(
                name = "accuracy_meters"
        )
        private Double accuracyMeters;


        @Column(
                name = "location_status",
                length = 32
        )
        private String locationStatus;

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

        this.locationStatus =
                "PENDING";
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

        public void markLocated(
                double latitude,
                double longitude,
                double accuracyMeters
        ) {

        this.latitude =
                latitude;

        this.longitude =
                longitude;

        this.accuracyMeters =
                accuracyMeters;

        this.locationStatus =
                "LOCATED";
        }


        public void markLocationStatus(
                String status
        ) {

        this.locationStatus =
                status;
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
        public Double getLatitude() {

        return latitude;
        }

        public Double getLongitude() {

        return longitude;
        }

        public Double getAccuracyMeters() {

        return accuracyMeters;
        }

        public String getLocationStatus() {

        return locationStatus;
        }       
}       