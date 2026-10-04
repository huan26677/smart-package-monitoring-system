package com.smartpackage.backend.controller;

import java.util.List;

import jakarta.validation.Valid;

import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.http.ResponseEntity;

import com.smartpackage.backend.dto.request.WifiLocationRequest;
import com.smartpackage.backend.dto.response.WifiLocationResponse;

import com.smartpackage.backend.service.WifiAnchorService;


@RestController
@RequestMapping("/api/wifi-locations")
public class WifiLocationController {


    private final WifiAnchorService
            wifiAnchorService;


    public WifiLocationController(
            WifiAnchorService wifiAnchorService
    ) {

        this.wifiAnchorService =
                wifiAnchorService;
    }


    @GetMapping
    public List<WifiLocationResponse>
            getAnchors() {

        return wifiAnchorService
                .getAnchors();
    }


    @PostMapping
    public WifiLocationResponse saveAnchor(

            @Valid
            @RequestBody
            WifiLocationRequest request

    ) {

        return wifiAnchorService
                .saveAnchor(
                        request
                );
    }


    @DeleteMapping
    public ResponseEntity<Void>
            deleteAnchor(

                    @RequestParam
                    String bssid

            ) {

        wifiAnchorService
                .deleteAnchor(
                        bssid
                );

        return ResponseEntity
                .noContent()
                .build();
    }
}