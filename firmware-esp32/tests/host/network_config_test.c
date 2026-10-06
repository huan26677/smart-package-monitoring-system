#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "network_config.h"

int main(void) {
    network_broker_t broker;
    assert(network_parse_broker("wss://mqtt.example.com/mqtt",&broker));
    assert(broker.tls && !broker.has_credentials && !strcmp(broker.uri,"wss://mqtt.example.com/mqtt"));
    assert(network_parse_broker("wss://device:p%40ss%3Aword@mqtt.example.com/mqtt",&broker));
    assert(broker.has_credentials && !strcmp(broker.username,"device") && !strcmp(broker.password,"p@ss:word"));
    assert(!strcmp(broker.uri,"wss://mqtt.example.com/mqtt"));
    assert(network_parse_broker("ws://example.com:8083",&broker));
    assert(!broker.tls && !strcmp(broker.uri,"ws://example.com:8083/mqtt"));
    assert(network_parse_broker("mqtt://192.168.1.5:1883",&broker));
    assert(network_parse_broker("mqtts://[::1]:8883",&broker));
    assert(network_parse_broker("mqtt://device@example.com",&broker));
    assert(broker.has_credentials && !broker.password[0]);
    const char *invalid[]={"", "https://example.com", "wss://", "mqtt://:1883", "mqtt://host:0",
        "mqtt://host:65536","mqtt://host:abc","mqtt://host:","mqtt://[::1","mqtt://[]", "mqtt://[::1]bad",
        "mqtt://one@two@host", "mqtt://:secret@host", "mqtt://u:%00@host", "mqtt://u:%zz@host",
        "wss://host/mqtt#fragment", "wss://ho st/mqtt", "wss://host\r\nX:bad", "wss://host\\mqtt"};
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) assert(!network_parse_broker(invalid[i],&broker));
    char overlong[300];memset(overlong,'a',sizeof(overlong));memcpy(overlong,"mqtt://",7);overlong[299]=0;
    assert(!network_parse_broker(overlong,&broker));
    char field[33];
    assert(network_form_value("ssid=WiFi+Moi&password=p%26a%2Bs%3Ds","ssid",field,sizeof(field))==1);
    assert(!strcmp(field,"WiFi Moi"));
    assert(network_form_value("password=p%26a%2Bs%3Ds","password",field,sizeof(field))==1);
    assert(!strcmp(field,"p&a+s=s"));
    assert(network_form_value("ssid=abc","broker",field,sizeof(field))==0 && !field[0]);
    assert(network_form_value("ssid=one&ssid=two","ssid",field,sizeof(field))==-1);
    assert(network_form_value("ssid=bad%","ssid",field,sizeof(field))==-1);
    assert(network_form_value("ssid=bad%0g","ssid",field,sizeof(field))==-1);
    assert(network_form_value("ssid=abc%00def","ssid",field,sizeof(field))==-1);
    assert(network_form_value("ssid=abc%0Adef","ssid",field,sizeof(field))==-1);
    assert(network_form_value("ssid=abcdefghijklmnopqrstuvwxyzABCDEF","ssid",field,sizeof(field))==1);
    assert(network_form_value("ssid=abcdefghijklmnopqrstuvwxyzABCDEFG","ssid",field,sizeof(field))==-1);
    assert(network_wifi_password_valid(""));
    assert(!network_wifi_password_valid("short"));
    assert(network_wifi_password_valid("12345678"));
    char hex[65];memset(hex,'a',64);hex[64]=0;
    assert(network_wifi_password_valid(hex));hex[63]='z';assert(!network_wifi_password_valid(hex));
    puts("network_config: URI, credentials, form decoding and Wi-Fi boundaries passed");
    return 0;
}
