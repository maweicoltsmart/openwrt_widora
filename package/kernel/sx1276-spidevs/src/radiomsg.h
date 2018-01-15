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

struct lora_server_data_list{
    uint8_t *buffer;
    uint16_t size;
    struct list_head list;
};

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

struct lora_tx_data{
    uint8_t *buffer;
    //uint32_t chip;
    uint16_t size;
    //int16_t rssi;
    //int8_t snr;
    uint8_t fPort;
    uint32_t addres;
    struct
    {
        uint8_t AckRequest      : 1;
        uint8_t Ack             : 1;
        uint8_t Reserve         : 6;
    }CtrlBits;
    struct list_head list;
};

void RadioMsgListInit(void);
int RadioRxMsgListAdd( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
int RadioRxMsgListGet(uint8_t *buf);
int Radio2ServerMsgListAdd( uint32_t addr,uint8_t fPort,uint8_t *payload, uint16_t size );
int Radio2ServerMsgListGet( uint8_t *buffer );
#endif