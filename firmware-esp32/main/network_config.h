#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H
#include <stdbool.h>
#include <stddef.h>

#define NETWORK_URI_MAX 256
#define NETWORK_USER_MAX 64
#define NETWORK_SECRET_MAX 128

typedef struct {
    char uri[NETWORK_URI_MAX + 1];
    char username[NETWORK_USER_MAX + 1];
    char password[NETWORK_SECRET_MAX + 1];
    bool has_credentials;
    bool tls;
} network_broker_t;

bool network_parse_broker(const char *uri, network_broker_t *result);
/* 1: found, 0: missing, -1: malformed, duplicated or too long. */
int network_form_value(const char *body, const char *key, char *out, size_t size);
bool network_wifi_password_valid(const char *password);
#endif
