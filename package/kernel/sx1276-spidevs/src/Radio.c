#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include "sx1276.h"
#include "sx1276-cdev.h"
#include "Radio.h"
#include "proc.h"
#include "debug.h"

//const struct Radio_s Radio;
DECLARE_WAIT_QUEUE_HEAD(lora_wait);
bool rx_done = false;
st_RadioRxList stRadioRxListHead;
static RadioEvents_t RadioEvent;

extern void RadioTxDone( int chip );
extern void RadioRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
extern void RadioTxTimeout( int chip );
extern void RadioRxTimeout( int chip );
extern void RadioRxError( int chip );

void RadioInit(void)
{
	int chip = 0;
	INIT_LIST_HEAD(&stRadioRxListHead.list);
	RadioEvent.TxDone = RadioTxDone;
    RadioEvent.RxDone = RadioRxDone;
    RadioEvent.RxError = RadioRxError;
    RadioEvent.TxTimeout = RadioTxTimeout;
    RadioEvent.RxTimeout = RadioRxTimeout;
    Radio.Init(0,&RadioEvent);
    Radio.Init(1,&RadioEvent);
    #if defined(GATEWAY_V2_3CHANNEL)
    Radio.Init(2,&RadioEvent);
    #endif
    Radio.Sleep(0);
    Radio.Sleep(1);
    #if defined(GATEWAY_V2_3CHANNEL)
    Radio.Sleep(2);
    #endif
    Radio.SetPublicNetwork(0,stRadioCfg_Rx.isPublic);
    Radio.SetPublicNetwork(1,stRadioCfg_Rx.isPublic);
    #if defined(GATEWAY_V2_3CHANNEL)
    Radio.SetPublicNetwork(2,stRadioCfg_Rx.isPublic);
    #endif
    chip = 0;
    Radio.SetTxConfig(chip,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stRadioCfg_Tx.datarate[chip],
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
    Radio.SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
    chip = 1;
    Radio.SetTxConfig(chip,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stRadioCfg_Tx.datarate[chip],
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
    Radio.SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
    #if defined(GATEWAY_V2_3CHANNEL)
    chip = 2;
    Radio.SetTxConfig(chip,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stRadioCfg_Tx.datarate[chip],
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
    Radio.SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
    #endif

    //radio_routin = kthread_create(Radio_routin, NULL, "Radio routin thread");
    //wake_up_process(radio_routin);
    return;
}

void RadioRemove(void)
{
	st_RadioRxList *tmp;
	struct list_head *pos, *q;

	Radio.Sleep(0);
    Radio.Sleep(1);
    #if defined(GATEWAY_V2_3CHANNEL)
    Radio.Sleep(2);
    #endif
    del_timer(&TxTimeoutTimer[0]);
    del_timer(&RxTimeoutTimer[0]);

    del_timer(&RxTimeoutSyncWord[0]);
    del_timer(&TxTimeoutTimer[1]);
    del_timer(&RxTimeoutTimer[1]);
    del_timer(&RxTimeoutSyncWord[1]);
    #if defined(GATEWAY_V2_3CHANNEL)
    del_timer(&TxTimeoutTimer[2]);
    del_timer(&RxTimeoutTimer[2]);
    #endif
    del_timer(&RxTimeoutSyncWord[2]);

    SX1276IoIrqFree(0);
    SX1276IoFree(0);
    SX1276IoIrqFree(1);
    SX1276IoFree(1);
    #if defined(GATEWAY_V2_3CHANNEL)
    SX1276IoIrqFree(2);
    SX1276IoFree(2);
    #endif

	list_for_each_safe(pos, q, &stRadioRxListHead.list){
		tmp= list_entry(pos, st_RadioRxList, list);
		kfree(tmp->stRadioRx.payload);
		list_del(pos);
		kfree(tmp);
	}
}
void RadioRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    RadioRxListAdd( chip,payload,size,rssi,snr );
    Radio.Sleep( chip);
    Radio.Rx( chip,0 );
    DEBUG_OUTPUT_EVENT(chip, EV_RXCOMPLETE);
    //DEBUG_OUTPUT_DATA(payload,size);
    rx_done = true;
    wake_up(&lora_wait);
}

void RadioTxDone( int chip )
{
    Radio.Sleep( chip);
    SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
    DEBUG_OUTPUT_EVENT(chip,EV_TXCOMPLETE);
    //State = TX;
}

void RadioTxTimeout( int chip )
{
    printk("%s , %d\r\n",__func__,chip);
	Radio.Sleep( chip);
    SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
}

void RadioRxTimeout( int chip )
{
    printk("%s , %d\r\n",__func__,chip);
    Radio.Sleep( chip);
    SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
}

void RadioRxError( int chip )
{
    Radio.Sleep( chip);
    SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[chip],
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[chip]]);
    Radio.Rx( chip,0 );
}

int RadioRxListAdd(int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){
    pst_RadioRxList new;
    new = (pst_RadioRxList)kmalloc(sizeof(st_RadioRxList),GFP_KERNEL);
    if(!new)
    {
        return -1;
    }
    new->stRadioRx.payload = kmalloc(size,GFP_KERNEL);
    if(!(new->stRadioRx.payload))
    {
        kfree(new);
        return -1;
    }
    new->jiffies = jiffies;
    new->stRadioRx.chip = chip;
    new->stRadioRx.size = size;
    new->stRadioRx.rssi = rssi;
    new->stRadioRx.snr = snr;
    memcpy(new->stRadioRx.payload,payload,size);
    list_add_tail(&(new->list), &stRadioRxListHead.list);
    return 0;
}
int RadioRxListGet(const pst_RadioRxList pstRadioRxList){
    pst_RadioRxList get;
    struct list_head *pos;
	uint8_t *buf = pstRadioRxList->stRadioRx.payload;
    if(list_empty(&stRadioRxListHead.list))
    {
        return -1;
    }
    else
    {
        pos = stRadioRxListHead.list.next;
        get = list_entry(pos, st_RadioRxList, list);
        memcpy(pstRadioRxList,get,sizeof(st_RadioRxList));
		pstRadioRxList->stRadioRx.payload = buf;
        memcpy(pstRadioRxList->stRadioRx.payload,get->stRadioRx.payload,get->stRadioRx.size);
		list_del(&get->list);
        kfree(get->stRadioRx.payload);
        kfree(get);
        return 0;
    }
}

