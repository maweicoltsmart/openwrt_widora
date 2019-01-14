#ifndef __GATEWAY_PRAGMA_H__
#define __GATEWAY_PRAGMA_H__
#include "nodedatabase.h"

#define MAX_IP_STRING_LENTH		(100)
typedef struct
{
	bool NetType;
    uint16_t slaveid;
    uint32_t baud;
    uint8_t parity;
    uint8_t softversion[64];
    uint8_t macaddress[6 * 2 + 1];
    uint8_t username[16 + 1];
    uint8_t password[16 + 1];
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    char server_ip[MAX_IP_STRING_LENTH];
    uint16_t server_port;
    uint32_t localip;
    uint16_t localport;
    struct Radio_t{
    	uint16_t index;
    	uint16_t channel;
    	uint16_t datarate;
    }radio[3];
}gateway_pragma_t;

extern gateway_pragma_t gateway_pragma;

void GetGatewayPragma(void);

#endif