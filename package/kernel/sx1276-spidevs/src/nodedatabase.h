#ifndef __NODE_DATA_BASE_H__
#define __NODE_DATA_BASE_H__
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include "utilities.h"
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/module.h> //Needed by all modules
#include <linux/kernel.h> //Needed for KERN_ALERT
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include "radiomsg.h"
#include "LoRaMac.h"

#define MAX_NODE    100

typedef enum
{
    enStateInit,
    enStateJoinning,
    enStateJoined,
}en_NodeState;

typedef struct
{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint8_t DevAddr[4];
    uint16_t DevNonce;
    struct timex LastComunication;
    uint32_t WakeupTime;
    uint32_t RX1_Window;
    uint32_t RX2_Window;
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
    uint8_t chip;
    struct timer_list *timer;
    uint32_t jiffies1;
    uint32_t jiffies2;
    uint32_t sequenceCounter_Down;
    uint32_t sequenceCounter_Up;
    struct lora_tx_data lora_tx_list;
    en_NodeState state;
    int16_t rssi;
    int8_t snr;
    bool is_ack_req;    // node reqest ack
    bool need_ack;  // gateway request ack
    uint8_t cmdbuf[LORA_MAC_COMMAND_MAX_LENGTH];
    uint8_t cmdlen;
    uint8_t *repeatbuf;
    uint16_t repeatlen;
}node_pragma_t;

typedef struct
{
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    uint8_t server_ip[4];
    uint16_t server_port;
}gateway_pragma_t;

typedef struct
{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
    uint8_t chip;
    int16_t rssi;
    int8_t snr;
}node_join_info_t;

void database_init(void);
int16_t database_node_join(node_join_info_t* node,uint32_t jiffiesval);
uint8_t verify_net_addr(uint32_t addr);
void get_msg_to_send( unsigned long index );
int RadioTxMsgListAdd(struct lora_tx_data *p);
void update_node_info(int index,int chip);


extern const gateway_pragma_t gateway_pragma;
extern node_pragma_t nodebase_node_pragma[];

#endif
