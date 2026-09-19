#ifndef __DEV_WIFI_H__
#define __DEV_WIFI_H__

#include "config.h"

#if DEVICE_WIFI

/* ==================== WiFi work modes ==================== */
typedef enum {
    OFF = 0,
    STA = 1,
    AP = 2,
    AP_STA = 3
} WorkMode;

/* ==================== Socket types ==================== */
typedef enum {
    UDPServer = 1,
    UDPClient = 2,
    TCPServer = 3,
    TCPClient = 4,
    TCPSeed = 5,
    SSLServer = 6,
    SSLClient = 7,
    SSLSeed = 8
} NetType;

/* ==================== API functions ==================== */
int WIFI_Init(void);
int WIFI_Connect(const char *name, const char *password);
int WIFI_Disconnect(void);
int WIFI_NetConnect(unsigned char type, const char *ip, unsigned int port);
int WIFI_NetDisconnect(unsigned int port);
int WIFI_Write(unsigned int port, const unsigned char *data, unsigned int length);
int WIFI_Read(unsigned int port, unsigned char *data, unsigned int length);

#endif /* DEVICE_WIFI */

#endif /* __DEV_WIFI_H__ */
