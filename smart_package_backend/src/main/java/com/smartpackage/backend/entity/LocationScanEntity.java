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

        @Column(
                name = "location_source",
                length = 32
        )
        private String locationSource;


        @Column(
                name = "location_label",
                length = 120
        )
        private String locationLabel;


        @Column(
                name = "matched_bssid",
                length = 17
        )
        private String matchedBssid;


        @Column(
                name = "matched_rssi"
        )
        private Integer matchedRssi;

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

public void markAnchorMatched(

        String label,

        String bssid,

        int rssi,

        double latitude,

        double longitude,

        double radiusMeters

) {

    this.latitude =
            latitude;

    this.longitude =
            longitude;

    this.accuracyMeters =
            radiusMeters;

    this.locationStatus =
            "ANCHOR_MATCHED";

    this.locationSource =
            "WIFI_ANCHOR";

    this.locationLabel =
            label;

    this.matchedBssid =
            bssid;

    this.matchedRssi =
            rssi;
}


        public void markNoAnchor() {

        this.locationStatus =
                "NO_ANCHOR";

        this.locationSource =
                "WIFI_ANCHOR";

        this.locationLabel =
                null;

        this.matchedBssid =
                null;

        this.matchedRssi =
                null;

        this.latitude =
                null;

        this.longitude =
                null;

        this.accuracyMeters =
                null;
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
        public String getLocationSource() {

        return locationSource;
        }


        public String getLocationLabel() {

        return locationLabel;
        }


        public String getMatchedBssid() {

        return matchedBssid;
        }


        public Integer getMatchedRssi() {

        return matchedRssi;
        }
}       