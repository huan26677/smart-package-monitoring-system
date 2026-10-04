package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.WifiLocationEntity;


public interface WifiLocationRepository
        extends JpaRepository<
                WifiLocationEntity,
                String
        > {
}