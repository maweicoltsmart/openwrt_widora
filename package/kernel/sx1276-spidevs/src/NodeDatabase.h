#ifndef __NODE_DATABASE_H__
#define __NODE_DATABASE_H__

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "Radio.h"
#include "LoRaMac.h"

#define MAX_NODE    1000

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
    uint32_t sequenceCounter_Down;
    uint32_t sequenceCounter_Up;
    uint8_t classtype;
}st_NodeInfoToSave;

typedef struct
{
    uint8_t buf[200 + 13];
    uint8_t len;
	//bool needack;
	unsigned long classcjiffies;
	//uint8_t fPort;
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
    unsigned long param;
    struct work_struct save;
}st_MyWork,*pst_MyWork;

typedef struct
{
    //st_NodeInfoToSave stNodeInfoToSave;
    struct timer_list timer1;
    struct timer_list timer2;
    unsigned long jiffies;
    st_TxData stTxData;
    en_NodeState state;
    int16_t rssi;
    int8_t snr;
    //bool is_ack_req;    // node reqest ack
    //bool have_ack;  // node have receiv
    uint16_t sequence_up;
	uint16_t sequence_down;
    uint8_t Battery;
	st_MyWork stMyWork;
	uint8_t chip;
}st_NodeDatabase,*pst_NodeDatabase;

typedef struct
{
    uint8_t AppEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
    uint8_t chip;
    int16_t rssi;
    int8_t snr;
    unsigned long jiffies;
	DeviceClass_t classtype;
}st_NodeJoinInfo,*pst_NodeJoinInfo;

extern st_NodeInfoToSave stNodeInfoToSave[];
extern st_NodeDatabase stNodeDatabase[];

void NodeDatabaseInit(void);
void NodeDatabaseRemove(void);
uint32_t NodeDatabaseJoin(const pst_NodeJoinInfo node);
void NodeDatabaseUpdateParameters(uint32_t addr, uint16_t fcnt, pst_RadioRxList pstRadioRxList);
bool NodeDatabaseVerifyNetAddr(uint32_t addr);
bool NodeGetNetAddr(uint32_t *addr,uint8_t *deveui);
bool NodeDatabaseGetDevEUI(uint32_t addr,uint8_t *deveui);
bool NodeDatabaseGetAppEUI(uint32_t addr,uint8_t *appeui);

#endif
