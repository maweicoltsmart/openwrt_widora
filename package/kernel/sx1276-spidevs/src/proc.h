#ifndef __LORA_PROC_H__
#define __LORA_PROC_H__

#include <linux/string.h>
#include "radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include <linux/spinlock.h>
#include "utilities.h"

#define LORA_PROCFS_NAME_LEN 32

typedef struct{
    int chip;
    RadioModems_t modem;
    uint32_t bandwidth;
    uint32_t datarate;
    uint8_t coderate;
    uint32_t bandwidthAfc;
    uint16_t preambleLen;
    uint16_t symbTimeout;
    bool fixLen;
    uint8_t payloadLen;
    bool crcOn;
    bool freqHopOn;
    uint8_t hopPeriod;
    bool iqInverted;
    bool rxContinuous;
}st_rxconfig,*pst_rxconfig;

typedef struct{
    int chip;
    RadioModems_t modem;
    int8_t power;
    uint32_t fdev;
    uint32_t bandwidth;
    uint32_t datarate;
    uint8_t coderate;
    uint16_t preambleLen;
    bool fixLen;
    bool crcOn;
    bool freqHopOn;
    uint8_t hopPeriod;
    bool iqInverted;
    uint32_t timeout;
}st_txconfig,*pst_txconfig;

typedef struct{
    char name[LORA_PROCFS_NAME_LEN + 1];
    union
    {
        st_rxconfig stRxconfig;
        st_txconfig stTxconfig;
    }proc_value;
}lora_cfg_proc_data_t;

typedef struct{
    uint32_t channel;
    uint32_t datarate;
}st_Channel,*pst_Channel;


typedef struct
{
    uint32_t channel;
    uint32_t freq_tx;
    uint32_t freq_rx;
    uint32_t dr_tx;
    uint32_t dr_rx;
    RadioModems_t modem;
    int8_t power;
    uint32_t fdev;
    uint32_t bandwidth;
    uint32_t datarate;
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
}st_RadioChip,*pst_RadioChip;

typedef struct
{
    uint32_t freq_tx[48];
    uint32_t freq_rx[96];
    uint32_t dr_range;
    uint32_t datarate;
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
}st_RadioCfg,*pst_RadioCfg;

extern bool rx_done;
extern st_RadioChip stRadioChip[];
extern st_RadioCfg stRadioCfg_Rx,stRadioCfg_Tx;

extern st_Channel stChannel[3];

int init_procfs_lora(void);
void cleanup_procfs_lora(void);

#endif
