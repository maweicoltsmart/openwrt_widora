#ifndef __LORA_DEV_OPS_H__

#include "radio.h"

#define CPU_SYS_TICK_HZ 100

typedef struct{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint32_t DevAddr;
    uint8_t fPort;
    uint8_t Battery;
    int16_t rssi;
    int8_t snr;
    uint8_t size;
    struct
    {
        uint8_t AckRequest      : 1;
        uint8_t Ack             : 1;
        uint8_t Reserve         : 6;
    }CtrlBits;
}lora_server_up_data_type;

typedef struct{
    uint8_t DevEUI[8];
    uint8_t fPort;
    uint8_t size;
    struct
    {
        uint8_t AckRequest      : 1;
        uint8_t Ack             : 1;
        uint8_t Reserve         : 6;
    }CtrlBits;
}lora_server_down_data_type;

void LoRaMacInit(void);
void *Radio_routin(void *param);

#endif