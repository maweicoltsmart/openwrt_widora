#ifndef __TYPE_DEF__
#define __TYPE_DEF__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "radio.h"
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif

#define BUFFER_SIZE 1024

extern bool lora_rx_done;
extern uint8_t lora_rx_len;
extern uint8_t radio2tcpbuffer[];

struct msg_st
{
    long int msg_type;
    char text[BUFFER_SIZE];
};

typedef struct
{
	uint32_t jiffies;
	uint32_t chip;
	uint32_t len;
}st_lora_rx_data_type,*pst_lora_rx_data_type;

typedef struct
{
	uint32_t jiffies_start;
	uint32_t jiffies_end;
	uint32_t chip;
	uint32_t len;
	struct timer_list *timer;
}st_lora_tx_data_type,*pst_lora_tx_data_type;

typedef struct
{
	uint32_t freq_tx[48];
	uint32_t freq_rx[96];
	uint32_t dr_range;
	uint32_t datarate[3];
	uint32_t channel[3];
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


#endif
