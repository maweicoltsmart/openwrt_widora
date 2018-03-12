#ifndef __NODE_DATABASE_H__
#define __NODE_DATABASE_H__

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include "Radio.h"

#define MAX_NODE    100

typedef struct
{
    uint8_t AppEUI[8];
    uint8_t DevEUI[8];
    uint32_t DevAddr;
    uint16_t DevNonce;
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
}st_DevNetParameter;

typedef struct
{
    st_DevNetParameter stDevNetParameter;
    uint8_t chip;
    uint32_t sequenceCounter_Down;
    uint32_t sequenceCounter_Up;
    uint8_t classtype;
}st_NodeInfoToSave;

typedef struct
{
    uint8_t *buf;
    uint8_t len;
}st_TxData;

typedef enum
{
    enStateInit,
    enStateJoinning,
    enStateJoinaccept1,
    enStateJoinaccept2,
    enStateJoined,
    enStateRxWindow1,
    enStateRxWindow2,
    enStateRxWindowOver
}en_NodeState;

typedef struct
{
    //st_NodeInfoToSave stNodeInfoToSave;
    struct timer_list timer1;
    struct timer_list timer2;
    uint32_t jiffies;
    st_TxData stTxData;
    en_NodeState state;
    int16_t rssi;
    int8_t snr;
    bool is_ack_req;    // node reqest ack
    bool have_ack;  // node have receiv
    uint8_t Battery;
}st_NodeDatabase,*pst_NodeDatabase;

typedef struct
{
    uint8_t AppEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
    uint8_t chip;
    int16_t rssi;
    int8_t snr;
    uint32_t jiffies;
	uint8_t classtype;
}st_NodeJoinInfo,*pst_NodeJoinInfo;

extern st_NodeInfoToSave stNodeInfoToSave[];
extern st_NodeDatabase stNodeDatabase[];

void NodeDatabaseInit(void);
void NodeDatabaseRemove(void);
uint32_t NodeDatabaseJoin(const pst_NodeJoinInfo node);
void NodeDatabaseUpdateParameters(uint32_t addr, pst_RadioRxList pstRadioRxList);
bool NodeDatabaseVerifyNetAddr(uint32_t addr);
bool NodeGetNetAddr(uint32_t *addr,uint8_t *deveui);
bool NodeDatabaseGetDevEUI(uint32_t addr,uint8_t *deveui);
bool NodeDatabaseGetAppEUI(uint32_t addr,uint8_t *appeui);

#endif
