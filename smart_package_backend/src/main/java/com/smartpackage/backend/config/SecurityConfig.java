package com.smartpackage.backend.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.core.userdetails.User;
import org.springframework.security.core.userdetails.UserDetailsService;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.security.provisioning.InMemoryUserDetailsManager;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
public class SecurityConfig {
    @Bean
    PasswordEncoder passwordEncoder() { return new BCryptPasswordEncoder(); }

    @Bean
    UserDetailsService users(@Value("${app.auth.username}") String username,
            @Value("${app.auth.password}") String password, PasswordEncoder encoder) {
        if (username.isBlank() || password.length() < 16
                || password.equals("replace_with_a_strong_password")) {
            throw new IllegalArgumentException("Set a dashboard password of at least 16 characters");
        }
        return new InMemoryUserDetailsManager(User.withUsername(username)
                .password(encoder.encode(password)).roles("ADMIN").build());
    }

    @Bean
    SecurityFilterChain security(HttpSecurity http) throws Exception {
        return http.authorizeHttpRequests(auth -> auth
                    .requestMatchers("/api/health", "/api/health/database", "/api/health/mqtt",
                            "/api/auth/csrf", "/api/auth/session", "/api/auth/login").permitAll()
                    .anyRequest().authenticated())
                .exceptionHandling(errors -> errors.accessDeniedHandler((req, res, ex) -> {
                    res.setStatus(403); res.setContentType("application/json;charset=UTF-8");
                    res.getWriter().write("{\"message\":\"Thiếu hoặc sai mã xác thực bảo mật\"}");
                }).authenticationEntryPoint((req, res, ex) -> {
                    res.setStatus(401); res.setContentType("application/json;charset=UTF-8");
                    res.getWriter().write("{\"message\":\"Vui lòng đăng nhập\"}");
                }))
                .formLogin(form -> form.loginProcessingUrl("/api/auth/login")
                    .successHandler((req, res, auth) -> {
                        res.setContentType("application/json;charset=UTF-8");
                        res.getWriter().write("{\"authenticated\":true}");
                    })
                    .failureHandler((req, res, ex) -> {
                        res.setStatus(401); res.setContentType("application/json;charset=UTF-8");
                        res.getWriter().write("{\"message\":\"Tài khoản hoặc mật khẩu không đúng\"}");
                    }))
                .logout(logout -> logout.logoutUrl("/api/auth/logout")
                    .logoutSuccessHandler((req, res, auth) -> res.setStatus(204)))
                .build();
    }
}
