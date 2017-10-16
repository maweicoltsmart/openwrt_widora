#ifndef __NODE_DATA_BASE_H__
#define __NODE_DATA_BASE_H__
#include "typedef.h"
#include <linux/timer.h> 
#include <linux/timex.h> 
#include <linux/rtc.h>
#include "aes.h"
#include "cmac.h"
#include "LoRaMacCrypto.h"

#define MAX_NODE	100

typedef struct
{
	uint8_t APPEUI[8];
	uint8_t DevEUI[8];
	uint8_t DevAddr[4];
	uint8_t DevNonce[2];
	struct timex LastComunication;
	uint32_t WakeupTime;
	uint32_t RX1_Window;
	uint32_t RX2_Window;
	uint8_t NwkSKey[16];
	uint8_t AppSKey[16];
}node_pragma_t;

typedef struct
{
	uint8_t APPKEY[16];
	uint8_t AppNonce[3];
	uint8_t NetID[4];
	uint8_t server_ip[4];
	uint16_t server_port;
}gateway_pragma_t;

typedef struct
{
	uint8_t APPEUI[8];
	uint8_t DevEUI[8];
	uint8_t DevNonce[2];
}node_join_info_t;


#endif
