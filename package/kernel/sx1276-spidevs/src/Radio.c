#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include "sx1276.h"
#include "sx1276-cdev.h"
#include "Radio.h"
#include "proc.h"
#include "debug.h"

#define RADIO_RX_TIMEOUT    (60 * HZ)
//const struct Radio_s Radio;
DECLARE_WAIT_QUEUE_HEAD(lora_wait);
bool rx_done = false;
st_RadioRxList stRadioRxListHead;
static RadioEvents_t RadioEvent;

struct mutex RadioChipMutex[4];

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
    
    chip = 0;
    mutex_init(&RadioChipMutex[chip]);
    Radio.Init(chip,&RadioEvent);
    Radio.Sleep(chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    Radio.SetPublicNetwork(chip,stRadioCfg_Rx.isPublic);
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
    mutex_init(&RadioChipMutex[chip]);
    Radio.Init(chip,&RadioEvent);
    Radio.Sleep(chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    Radio.SetPublicNetwork(chip,stRadioCfg_Rx.isPublic);
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
    chip = 2;
    mutex_init(&RadioChipMutex[chip]);
    Radio.Init(chip,&RadioEvent);
    Radio.Sleep(chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    Radio.SetPublicNetwork(chip,stRadioCfg_Rx.isPublic);
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
    Radio.Sleep(chip);
    chip = 3;
    mutex_init(&RadioChipMutex[chip]);
    Radio.Init(chip,&RadioEvent);
    Radio.Sleep(chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    Radio.SetPublicNetwork(chip,stRadioCfg_Rx.isPublic);
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
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[25]);
    Radio.Rx( chip,0 );
    Radio.Sleep(chip);
    return;
}

void RadioRemove(void)
{
	st_RadioRxList *tmp;
	struct list_head *pos, *q;

	Radio.Sleep(0);
    Radio.Sleep(1);
    Radio.Sleep(2);
    Radio.Sleep(3);
    del_timer(&TxTimeoutTimer[0]);
    del_timer(&RxTimeoutTimer[0]);
    del_timer(&RxTimeoutSyncWord[0]);
    del_timer(&TxTimeoutTimer[1]);
    del_timer(&RxTimeoutTimer[1]);
    del_timer(&RxTimeoutSyncWord[1]);
    del_timer(&TxTimeoutTimer[2]);
    del_timer(&RxTimeoutTimer[2]);
    del_timer(&RxTimeoutSyncWord[2]);
    del_timer(&TxTimeoutTimer[3]);
    del_timer(&RxTimeoutTimer[3]);
    del_timer(&RxTimeoutSyncWord[3]);

    SX1276IoIrqFree(0);
    SX1276IoFree(0);
    SX1276IoIrqFree(1);
    SX1276IoFree(1);
    SX1276IoIrqFree(2);
    SX1276IoFree(2);
    SX1276IoIrqFree(3);
    SX1276IoFree(3);

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
    /*if(Radio.GetStatus(chip) == RF_TX_RUNNING)
    {
        return;
    }*/
    Radio.Sleep( chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    if(chip < 2)
    {
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
        Radio.Rx( chip,RADIO_RX_TIMEOUT );
    }
    DEBUG_OUTPUT_EVENT(chip, EV_RXCOMPLETE);
    printk("%s ,%d\r\n",__func__,chip);
    //DEBUG_OUTPUT_DATA(payload,size);
    rx_done = true;
    wake_up(&lora_wait);
}

void RadioTxDone( int chip )
{
    /*if(Radio.GetStatus(chip) == RF_TX_RUNNING)
    {
        return;
    }*/
    Radio.Sleep( chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    /*if(chip < 3)
    {
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
        Radio.Rx( chip,RADIO_RX_TIMEOUT );
    }*/
    DEBUG_OUTPUT_EVENT(chip,EV_TXCOMPLETE);
    printk("%s, %d\r\n",__func__,chip);
    //State = TX;
}

void RadioTxTimeout( int chip )
{
    printk("%s , %d\r\n",__func__,chip);
	Radio.Sleep( chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    /*if(chip < 2)
    {
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
        Radio.Rx( chip,RADIO_RX_TIMEOUT );
    }*/
}

void RadioRxTimeout( int chip )
{
    printk("%s , %d\r\n",__func__,chip);
    Radio.Sleep( chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    if(chip < 2)
    {
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
        Radio.Rx( chip,RADIO_RX_TIMEOUT );
    }
}

void RadioRxError( int chip )
{
    /*if(Radio.GetStatus(chip) == RF_TX_RUNNING)
    {
        return;
    }*/
    Radio.Sleep( chip);
    Radio.SetMaxPayloadLength(chip,MODEM_LORA,0xff);
    if(chip < 2)
    {
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
        Radio.Rx( chip,RADIO_RX_TIMEOUT );
    }
    printk("%s ,%d\r\n",__func__,chip);
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
    if(size > (200 + 13))
    {
        size = (200 + 13);
    }
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

