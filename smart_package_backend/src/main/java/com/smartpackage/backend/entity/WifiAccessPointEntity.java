package com.smartpackage.backend.entity;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;


@Entity
@Table(name = "wifi_access_points")
public class WifiAccessPointEntity {


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
            name = "location_scan_id",
            nullable = false
    )
    private LocationScanEntity locationScan;


    @Column(
            name = "bssid",
            length = 17,
            nullable = false
    )
    private String bssid;


    @Column(
            name = "rssi",
            nullable = false
    )
    private int rssi;


    protected WifiAccessPointEntity() {
    }


    public WifiAccessPointEntity(
            String bssid,
            int rssi
    ) {

        this.bssid =
                bssid;

        this.rssi =
                rssi;
    }


    void setLocationScan(
            LocationScanEntity locationScan
    ) {

        this.locationScan =
                locationScan;
    }


    public Long getId() {

        return id;
    }


    public LocationScanEntity
            getLocationScan() {

        return locationScan;
    }


    public String getBssid() {

        return bssid;
    }


    public int getRssi() {

        return rssi;
    }
}