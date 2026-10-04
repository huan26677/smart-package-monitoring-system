package com.smartpackage.backend.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.smartpackage.backend.entity.WifiAccessPointEntity;


public interface WifiAccessPointRepository
        extends JpaRepository<
                WifiAccessPointEntity,
                Long
        > {
}