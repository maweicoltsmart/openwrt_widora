#ifndef __LORAWAN_H__
#define __LORAWAN_H__

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include "LoRaMacCrypto.h"
#include "LoRaMac.h"
#include "Radio.h"
#include "NodeDatabase.h"
#include "Server.h"
#include "debug.h"

typedef struct
{
    uint8_t AppKey[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
}st_GatewayParameter;

typedef struct
{
    LoRaMacHeader_t macHdr;
    uint8_t AppEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
    uint32_t mic;
}st_FrameJoinRequest,*pst_FrameJoinRequest;

typedef struct
{
    LoRaMacHeader_t macHdr;
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    uint32_t DevAddr;
    uint8_t DLSettings;
    uint8_t RxDelay;
    uint32_t mic;
}st_FrameJoinAccept,*pst_FrameJoinAccept;

typedef struct
{
	uint32_t DevAddr;
	LoRaMacFrameCtrl_t Fctrl;
	uint16_t Fcnt;
}LoRaFrameHeader_t;

typedef struct
{
    LoRaMacHeader_t macHdr;
	LoRaFrameHeader_t frameHdr;
    uint32_t mic;
}st_FrameDataUp,*pst_FrameDataUp;

typedef struct
{
    LoRaMacHeader_t macHdr;
	LoRaFrameHeader_t frameHdr;
    uint32_t mic;
}st_FrameDataDown,*pst_FrameDataDown;

extern st_GatewayParameter stGatewayParameter;

void LoRaWANInit(void);
void LoRaWANRemove(void);
int LoRaWANRxDataProcess(void *data);

#endif
