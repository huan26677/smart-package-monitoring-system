package com.smartpackage.backend.exception;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.Map;

import jakarta.servlet.http.HttpServletRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;

import org.springframework.http.converter.HttpMessageNotReadableException;

import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.MissingServletRequestParameterException;

import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import org.springframework.web.method.annotation.MethodArgumentTypeMismatchException;

import org.springframework.web.server.ResponseStatusException;

import com.smartpackage.backend.dto.response.ApiErrorResponse;

@RestControllerAdvice
public class GlobalExceptionHandler {

    private static final Logger logger =
            LoggerFactory.getLogger(
                    GlobalExceptionHandler.class
            );

    /* =====================================================
     * RESPONSE STATUS EXCEPTION
     * ===================================================== */

    @ExceptionHandler(
            ResponseStatusException.class
    )
    public ResponseEntity<ApiErrorResponse>
            handleResponseStatusException(

                    ResponseStatusException ex,

                    HttpServletRequest request

            ) {

        int status =
                ex.getStatusCode()
                        .value();

        HttpStatus httpStatus =
                HttpStatus.resolve(
                        status
                );

        String error =
                httpStatus != null
                        ? httpStatus.getReasonPhrase()
                        : "HTTP Error";

        String message =
                ex.getReason() != null
                        ? ex.getReason()
                        : error;

        return ResponseEntity
                .status(
                        status
                )
                .body(
                        createError(

                                status,

                                error,

                                message,

                                request,

                                Map.of()
                        )
                );
    }

    /* =====================================================
     * VALIDATION
     * ===================================================== */

    @ExceptionHandler(
            MethodArgumentNotValidException.class
    )
    public ResponseEntity<ApiErrorResponse>
            handleValidation(

                    MethodArgumentNotValidException ex,

                    HttpServletRequest request

            ) {

        Map<String, String> fieldErrors =
                new LinkedHashMap<>();

        ex.getBindingResult()
                .getFieldErrors()
                .forEach(
                        fieldError ->

                                fieldErrors
                                        .putIfAbsent(

                                                fieldError.getField(),

                                                fieldError
                                                        .getDefaultMessage()
                                        )
                );

        return ResponseEntity
                .badRequest()
                .body(
                        createError(

                                HttpStatus.BAD_REQUEST.value(),

                                HttpStatus.BAD_REQUEST
                                        .getReasonPhrase(),

                                "Validation failed",

                                request,

                                fieldErrors
                        )
                );
    }

    /* =====================================================
     * BAD REQUEST
     * ===================================================== */

    @ExceptionHandler(
            IllegalArgumentException.class
    )
    public ResponseEntity<ApiErrorResponse>
            handleIllegalArgument(

                    IllegalArgumentException ex,

                    HttpServletRequest request

            ) {

        return ResponseEntity
                .badRequest()
                .body(
                        createError(

                                HttpStatus.BAD_REQUEST.value(),

                                HttpStatus.BAD_REQUEST
                                        .getReasonPhrase(),

                                ex.getMessage(),

                                request,

                                Map.of()
                        )
                );
    }

    @ExceptionHandler(
            {
                    MethodArgumentTypeMismatchException.class,
                    MissingServletRequestParameterException.class,
                    HttpMessageNotReadableException.class
            }
    )
    public ResponseEntity<ApiErrorResponse>
            handleInvalidRequest(

                    Exception ex,

                    HttpServletRequest request

            ) {

        return ResponseEntity
                .badRequest()
                .body(
                        createError(

                                HttpStatus.BAD_REQUEST.value(),

                                HttpStatus.BAD_REQUEST
                                        .getReasonPhrase(),

                                "Invalid request",

                                request,

                                Map.of()
                        )
                );
    }

    /* =====================================================
     * UNKNOWN ERROR
     * ===================================================== */

    @ExceptionHandler(
            Exception.class
    )
    public ResponseEntity<ApiErrorResponse>
            handleException(

                    Exception ex,

                    HttpServletRequest request

            ) {

        logger.error(
                "Unhandled backend exception",
                ex
        );

        return ResponseEntity
                .status(
                        HttpStatus.INTERNAL_SERVER_ERROR
                )
                .body(
                        createError(

                                HttpStatus
                                        .INTERNAL_SERVER_ERROR
                                        .value(),

                                HttpStatus
                                        .INTERNAL_SERVER_ERROR
                                        .getReasonPhrase(),

                                "Internal server error",

                                request,

                                Map.of()
                        )
                );
    }

    /* =====================================================
     * HELPER
     * ===================================================== */

    private ApiErrorResponse createError(

            int status,

            String error,

            String message,

            HttpServletRequest request,

            Map<String, String> fieldErrors

    ) {

        return new ApiErrorResponse(

                Instant.now(),

                status,

                error,

                message,

                request.getRequestURI(),

                fieldErrors
        );
    }
}
