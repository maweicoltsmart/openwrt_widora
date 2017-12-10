#ifndef __LORA_DEV_OPS_H__

#include "radio.h"

#define CPU_SYS_TICK_HZ 100

void LoRaMacInit(void);
void *Radio_routin(void *param);

#endif