#ifndef __LORA_PROC_H__
#define __LORA_PROC_H__

#include <linux/string.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include "utilities.h"
#include "Radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include "pinmap.h"

typedef struct
{
	uint32_t freq_tx[48];
	uint32_t freq_rx[96];
	uint32_t dr_range;
	uint32_t datarate[4];
	uint32_t channel[4];
	RadioModems_t modem;
    int8_t power;
    uint32_t fdev;
    uint32_t bandwidth;
    uint8_t coderate;
    uint16_t preambleLen;
    bool fixLen;
    bool crcOn;
    bool freqHopOn;
    uint8_t hopPeriod;
    bool iqInverted;
    uint32_t timeout;
    uint32_t bandwidthAfc;
    uint16_t symbTimeout;
    uint8_t payloadLen;
    bool rxContinuous;
	bool isPublic;
}st_RadioCfg,*pst_RadioCfg;

typedef struct
{
	uint8_t APPKEY[16];
	uint8_t NetID[3];
	uint8_t AppNonce[3];
}st_MacCfg,*pst_MacCfg;

extern st_RadioCfg stRadioCfg_Rx,stRadioCfg_Tx;
extern st_MacCfg stMacCfg;

int init_procfs_lora(void);
void cleanup_procfs_lora(void);

#endif
