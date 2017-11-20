#ifndef __LORA_DEV_OPS_H__

#include "radio.h"

#define CPU_SYS_TICK_HZ 100
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
    char name[32 + 1];
    union
    {
        st_rxconfig stRxconfig;
        st_txconfig stTxconfig;
    }proc_value;
}lora_proc_data_t;

typedef struct{
    uint32_t freq;
    uint32_t datarate;
}st_channel,*pst_channel;

void LoRaMacInit(void);
void *Radio_routin(void *param);

#endif