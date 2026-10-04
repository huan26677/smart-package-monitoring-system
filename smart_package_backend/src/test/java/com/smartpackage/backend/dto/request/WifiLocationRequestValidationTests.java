package com.smartpackage.backend.dto.request;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import jakarta.validation.ConstraintViolation;
import jakarta.validation.Validation;
import jakarta.validation.Validator;
import jakarta.validation.ValidatorFactory;

class WifiLocationRequestValidationTests {

    private static ValidatorFactory
            validatorFactory;

    private static Validator
            validator;

    @BeforeAll
    static void setUpValidator() {

        validatorFactory =
                Validation
                        .buildDefaultValidatorFactory();

        validator =
                validatorFactory
                        .getValidator();
    }

    @AfterAll
    static void closeValidator() {

        validatorFactory.close();
    }

    @Test
    void validRequestShouldHaveNoViolations() {

        WifiLocationRequest request =
                new WifiLocationRequest(

                        "AA:BB:CC:DD:EE:FF",

                        "Kho A",

                        10.9800,

                        106.6700,

                        null
                );

        Set<ConstraintViolation<WifiLocationRequest>>
                violations =
                        validator.validate(
                                request
                        );

        assertTrue(
                violations.isEmpty()
        );
    }

    @Test
    void invalidRequestShouldReportExpectedFields() {

        WifiLocationRequest request =
                new WifiLocationRequest(

                        "ABC",

                        "",

                        999.0,

                        999.0,

                        null
                );

        Set<ConstraintViolation<WifiLocationRequest>>
                violations =
                        validator.validate(
                                request
                        );

        Set<String> fields =
                violations
                        .stream()
                        .map(
                                violation ->
                                        violation
                                                .getPropertyPath()
                                                .toString()
                        )
                        .collect(
                                Collectors.toSet()
                        );

        assertTrue(
                fields.contains(
                        "bssid"
                )
        );

        assertTrue(
                fields.contains(
                        "name"
                )
        );

        assertTrue(
                fields.contains(
                        "latitude"
                )
        );

        assertTrue(
                fields.contains(
                        "longitude"
                )
        );
    }

    @Test
    void negativeRadiusShouldFailValidation() {

        WifiLocationRequest request =
                new WifiLocationRequest(

                        "AA:BB:CC:DD:EE:FF",

                        "Kho A",

                        10.9800,

                        106.6700,

                        -1.0
                );

        Set<ConstraintViolation<WifiLocationRequest>>
                violations =
                        validator.validate(
                                request
                        );

        long radiusErrors =
                violations
                        .stream()
                        .filter(
                                violation ->
                                        violation
                                                .getPropertyPath()
                                                .toString()
                                                .equals(
                                                        "radiusMeters"
                                                )
                        )
                        .count();

        assertEquals(
                1,
                radiusErrors
        );
    }
}
