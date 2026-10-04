package com.smartpackage.backend.entity;

import java.time.Instant;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.PrePersist;
import jakarta.persistence.Table;


@Entity
@Table(name = "wifi_locations")
public class WifiLocationEntity {


    @Id
    @Column(
            name = "bssid",
            length = 17,
            nullable = false
    )
    private String bssid;


    @Column(
            name = "name",
            length = 120,
            nullable = false
    )
    private String name;


    @Column(
            name = "latitude",
            nullable = false
    )
    private double latitude;


    @Column(
            name = "longitude",
            nullable = false
    )
    private double longitude;


    @Column(
            name = "radius_meters",
            nullable = false
    )
    private double radiusMeters;


    @Column(
            name = "created_at",
            nullable = false
    )
    private Instant createdAt;


    protected WifiLocationEntity() {
    }


    public WifiLocationEntity(
            String bssid,
            String name,
            double latitude,
            double longitude,
            double radiusMeters
    ) {

        this.bssid =
                bssid.toUpperCase();

        this.name =
                name;

        this.latitude =
                latitude;

        this.longitude =
                longitude;

        this.radiusMeters =
                radiusMeters;
    }


    @PrePersist
    private void prePersist() {

        if (createdAt == null) {

            createdAt =
                    Instant.now();
        }
    }


    public void update(
            String name,
            double latitude,
            double longitude,
            double radiusMeters
    ) {

        this.name =
                name;

        this.latitude =
                latitude;

        this.longitude =
                longitude;

        this.radiusMeters =
                radiusMeters;
    }


    public String getBssid() {

        return bssid;
    }


    public String getName() {

        return name;
    }


    public double getLatitude() {

        return latitude;
    }


    public double getLongitude() {

        return longitude;
    }


    public double getRadiusMeters() {

        return radiusMeters;
    }


    public Instant getCreatedAt() {

        return createdAt;
    }
}