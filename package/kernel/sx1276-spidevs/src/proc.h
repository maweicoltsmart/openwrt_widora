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
}lora_proc_data_t;

int init_procfs_lora(void);
void cleanup_procfs_lora(void);

#endif
