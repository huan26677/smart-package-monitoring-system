#include "network_config.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static int hex(char c) {
    if(c >= '0' && c <= '9') return c-'0';
    if(c >= 'a' && c <= 'f') return c-'a'+10;
    if(c >= 'A' && c <= 'F') return c-'A'+10;
    return -1;
}

static bool decode(const char *src, size_t length, char *out, size_t size, bool form) {
    size_t n=0;
    for(size_t i=0;i<length;i++) {
        unsigned char c=(unsigned char)src[i];
        if(c=='%') {
            if(i+2>=length || hex(src[i+1])<0 || hex(src[i+2])<0) return false;
            c=(unsigned char)((hex(src[i+1])<<4)|hex(src[i+2])); i+=2;
        } else if(form && c=='+') c=' ';
        if(c<32 || c==127 || n+1>=size) return false;
        out[n++]=(char)c;
    }
    out[n]='\0'; return true;
}

int network_form_value(const char *body,const char *key,char *out,size_t size) {
    if(!body || !key || !out || !size) return -1;
    out[0]='\0'; int found=0; size_t keylen=strlen(key);
    for(const char *p=body;*p;) {
        const char *end=strchr(p,'&'); if(!end) end=p+strlen(p);
        const char *equal=memchr(p,'=',(size_t)(end-p));
        if(equal && (size_t)(equal-p)==keylen && !memcmp(p,key,keylen)) {
            if(found || !decode(equal+1,(size_t)(end-equal-1),out,size,true)) return -1;
            found=1;
        }
        p=*end?end+1:end;
    }
    return found;
}

bool network_parse_broker(const char *uri, network_broker_t *out) {
    if(!uri || !out || !uri[0] || strlen(uri)>NETWORK_URI_MAX) return false;
    memset(out,0,sizeof(*out));
    const char *authority=NULL; bool ws=false;
    if(!strncmp(uri,"mqtt://",7)) authority=uri+7;
    else if(!strncmp(uri,"mqtts://",8)) {authority=uri+8;out->tls=true;}
    else if(!strncmp(uri,"ws://",5)) {authority=uri+5;ws=true;}
    else if(!strncmp(uri,"wss://",6)) {authority=uri+6;ws=true;out->tls=true;}
    else return false;
    for(const unsigned char *p=(const unsigned char*)uri;*p;p++)
        if(*p<=32 || *p>=127 || *p=='#' || *p=='\\') return false;
    const char *end=authority+strcspn(authority,"/?");
    const char *host=authority;
    const char *at=memchr(authority,'@',(size_t)(end-authority));
    if(at) {
        if(memchr(at+1,'@',(size_t)(end-at-1))) return false;
        const char *colon=memchr(authority,':',(size_t)(at-authority));
        const char *user_end=colon?colon:at;
        if(!decode(authority,(size_t)(user_end-authority),out->username,sizeof(out->username),false) || !out->username[0]) return false;
        if(colon && !decode(colon+1,(size_t)(at-colon-1),out->password,sizeof(out->password),false)) return false;
        out->has_credentials=true; host=at+1;
    }
    if(host==end) return false;
    const char *port=NULL;
    if(*host=='[') {
        const char *close=memchr(host,']',(size_t)(end-host));
        if(!close || close==host+1) return false;
        for(const char *p=host+1;p<close;p++) if(!isxdigit((unsigned char)*p) && *p!=':' && *p!='.') return false;
        if(close+1<end) {if(close[1]!=':') return false;port=close+2;}
    } else {
        const char *colon=memchr(host,':',(size_t)(end-host));
        const char *host_end=colon?colon:end;
        if(host==host_end) return false;
        for(const char *p=host;p<host_end;p++) if(!isalnum((unsigned char)*p) && *p!='-' && *p!='.') return false;
        if(colon) port=colon+1;
    }
    if(port) {
        unsigned value=0;
        if(port==end) return false;
        for(const char *p=port;p<end;p++) {if(!isdigit((unsigned char)*p)) return false;value=value*10+(unsigned)(*p-'0');if(value>65535) return false;}
        if(!value) return false;
    }
    size_t prefix=(size_t)(authority-uri), remainder=strlen(host);
    const char *suffix=ws && end[0]=='\0'?"/mqtt":"";
    if(prefix+remainder+strlen(suffix)>NETWORK_URI_MAX) return false;
    memcpy(out->uri,uri,prefix); memcpy(out->uri+prefix,host,remainder+1);
    strcat(out->uri,suffix); return true;
}

bool network_wifi_password_valid(const char *password) {
    if(!password) return false;
    size_t len=strlen(password);
    if(!len) return true;
    if(len>=8 && len<=63) return true;
    if(len!=64) return false;
    for(size_t i=0;i<len;i++) if(hex(password[i])<0) return false;
    return true;
}
