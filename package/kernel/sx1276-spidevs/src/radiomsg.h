#ifndef __RADIO_MSG_H__
#define __RADIO_MSG_H__

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/list.h>

struct lora_rx_data{
    uint8_t *buffer;
    uint32_t jiffies;
    uint32_t chip;
    uint16_t size;
    int16_t rssi;
    int8_t snr;
    struct list_head list;
};

struct lora_tx_data{
    uint8_t *buffer;
    //uint32_t chip;
    uint16_t size;
    //int16_t rssi;
    //int8_t snr;
    uint8_t fPort;
    uint32_t addres;
    struct list_head list;
};

void RadioMsgListInit(void);
int RadioRxMsgListAdd( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
int RadioRxMsgListGet(uint8_t *buf);

#endif