#ifndef __GATEWAY_PRAGMA_H__
#define __GATEWAY_PRAGMA_H__
#include "nodedatabase.h"

typedef struct
{
	bool NetType;
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    uint8_t server_ip[4];
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