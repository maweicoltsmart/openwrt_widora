#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include "LoRaWAN.h"
#include "proc.h"
#include "utilities.h"

struct workqueue_struct *RadioWorkQueue;
LoRaMacParams_t LoRaMacParams;
st_GatewayParameter stGatewayParameter;
static struct task_struct *radio_routin;

typedef void (*pdotasklet)(struct work_struct *p_work);

extern void LoRaWANInit(void);
extern int LoRaWANRxDataProcess(void *data);
extern void LoRaWANJoinTimer1Callback( unsigned long index );
extern void LoRaWANJoinTimer2Callback( unsigned long index );
void LoRaWANDataDownTimer1Callback( unsigned long index );
void LoRaWANDataDownTimer2Callback( unsigned long index );
void LoRaWANDataDownWaitAckTimer2Callback( unsigned long index );
void LoRaWANDataDownClassCTimer1Callback( unsigned long index );
void LoRaWANDataDownClassCTimer2Callback( unsigned long index );
void LoRaWANDataDownClassCWaitAckTimer2Callback( unsigned long index );
void LoRaWANRadomDataDownClassCTimer2Callback( unsigned long index );

extern void LoRaWANJoinAccept(uint32_t addr);

void LoRaWANInit(void)
{
    LoRaMacParams.JoinAcceptDelay1 = 1 * HZ - 11 - 3;
    LoRaMacParams.JoinAcceptDelay2 = 3 * HZ - 3 - 3;
    LoRaMacParams.ReceiveDelay1 = 1 * HZ - 11 - 3;
    LoRaMacParams.ReceiveDelay2 = 3 * HZ - 3 - 3;
	memcpy(stGatewayParameter.AppKey,stMacCfg.APPKEY,16);
	DEBUG_OUTPUT_DATA((unsigned char *)stGatewayParameter.AppKey,16);
	memcpy(stGatewayParameter.NetID,stMacCfg.NetID,3);
	//memcpy(stGatewayParameter.AppNonce,stMacCfg.AppNonce,3);
	ServerMsgInit();
	NodeDatabaseInit();
    RadioWorkQueue = create_workqueue("test_workqueue");
	RadioInit();
    srand1(8);
	radio_routin = kthread_create(LoRaWANRxDataProcess, NULL, "LoRaWANRxDataProcess");
    wake_up_process(radio_routin);
}

void LoRaWANRemove(void)
{
	kthread_stop(radio_routin);
	RadioRemove();
	ServerMsgRemove();
	NodeDatabaseRemove();
    destroy_workqueue(RadioWorkQueue);
}

int LoRaWANRxDataProcess(void *data){
	st_ServerMsgUp stServerMsgUp;
	st_RadioRxList stRadioRxList;
	uint8_t payloadRx[256];
	uint8_t payloadTx[256];
	st_FrameDataUp stFrameDataUp;
	st_NodeJoinInfo stNodeJoinInfo;
	LoRaMacHeader_t macHdr;
	uint32_t micRx = 0;
	uint8_t appPayloadStartIndex = 0;
    uint8_t port = 0xFF;
    uint8_t frameLen = 0;
	int32_t addr = 0;
    int err;
    bool watchdog = false;

    stRadioRxList.stRadioRx.payload = payloadRx;
    err = gpio_request(WDT_REED_PIN, "WDT_REED_PIN");
    if(err)
    {
        printk("watchdog pin request error\r\n");
    }
    err = gpio_direction_output(WDT_REED_PIN,watchdog);
    if(err)
    {
        printk("watchdog pin output error\r\n");
    }
    gpio_set_value(WDT_REED_PIN,watchdog);
    while (true)
    {
        watchdog = !watchdog;
        gpio_set_value(WDT_REED_PIN,watchdog);
        if( kthread_should_stop())
        {
            return -1;
        }
        if(RadioRxListGet(&stRadioRxList) < 0)
        {
            rx_done = false;
            //wait_event_interruptible(lora_wait,rx_done);
            wait_event_interruptible_timeout(lora_wait, rx_done, HZ / 5);
        }
        else
        {
        	macHdr.Value = stRadioRxList.stRadioRx.payload[0];
			switch(macHdr.Bits.MType)
			{
				case FRAME_TYPE_JOIN_REQ:
					LoRaMacJoinComputeMic( stRadioRxList.stRadioRx.payload, 19, stGatewayParameter.AppKey, &micRx );
					if(*(unsigned long*)&stRadioRxList.stRadioRx.payload[19] == micRx)
					{
						memcpy(stNodeJoinInfo.AppEUI,stRadioRxList.stRadioRx.payload + 1,8);
						memcpy(stNodeJoinInfo.DevEUI,stRadioRxList.stRadioRx.payload + 1 + 8,8);
						stNodeJoinInfo.DevNonce = *(uint16_t*)(stRadioRxList.stRadioRx.payload + 1 + 8 + 8);
						stNodeJoinInfo.chip = stRadioRxList.stRadioRx.chip;
						stNodeJoinInfo.rssi= stRadioRxList.stRadioRx.rssi;
						stNodeJoinInfo.snr= stRadioRxList.stRadioRx.snr;
						stNodeJoinInfo.jiffies = stRadioRxList.jiffies;
						stNodeJoinInfo.classtype = macHdr.Bits.RFU;
                        uint32_t random = randr(0x00,0xffffff);
                        stGatewayParameter.AppNonce[0] = (random & 0xff)  >> 0;
                        stGatewayParameter.AppNonce[1] = (random & 0xff00)  >> 1;
                        stGatewayParameter.AppNonce[2] = (random & 0xff0000)  >> 2;

                        addr = NodeDatabaseJoin(&stNodeJoinInfo);
						if(addr < 0)
						{
							printk("%s,%d\r\n",__func__,__LINE__);
						}
						else	// Send join accept fram
						{
							LoRaWANJoinAccept(addr);
						}
					}
					break;
				case FRAME_TYPE_DATA_UNCONFIRMED_UP:
				case FRAME_TYPE_DATA_CONFIRMED_UP:
                    stServerMsgUp.Msg.stData2Server.size = 0;
                    port = 0xff;
					stFrameDataUp.macHdr.Value = stRadioRxList.stRadioRx.payload[0];
					stFrameDataUp.frameHdr.DevAddr = *(uint32_t*)&stRadioRxList.stRadioRx.payload[1];
					stFrameDataUp.frameHdr.Fctrl.Value = stRadioRxList.stRadioRx.payload[1 + 4];
					stFrameDataUp.frameHdr.Fcnt = *(uint16_t*)&stRadioRxList.stRadioRx.payload[1 + 4 + 1];
					stFrameDataUp.mic = *(uint32_t*)&stRadioRxList.stRadioRx.payload[stRadioRxList.stRadioRx.size - 4];
					appPayloadStartIndex = 8 + stFrameDataUp.frameHdr.Fctrl.Bits.FOptsLen;
					if(!NodeDatabaseVerifyNetAddr(stFrameDataUp.frameHdr.DevAddr))
					{
						//printk("%s,%d,%d\r\n",__func__,__LINE__,stFrameDataUp.frameHdr.DevAddr);
						break;
					}
					LoRaMacComputeMic( stRadioRxList.stRadioRx.payload, stRadioRxList.stRadioRx.size - LORAMAC_MFR_LEN, stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.NwkSKey, stFrameDataUp.frameHdr.DevAddr, UP_LINK, stFrameDataUp.frameHdr.Fcnt, &micRx );
					if(stFrameDataUp.mic != micRx)
					{
						printk("error: mic! 0x%08X, 0x%08X\r\n",stFrameDataUp.mic,micRx);
						break;
					}
					printk("%s,%d,%d,%d\r\n",__func__,__LINE__,stFrameDataUp.frameHdr.DevAddr,stRadioRxList.stRadioRx.chip);
					NodeDatabaseUpdateParameters(stFrameDataUp.frameHdr.DevAddr,stFrameDataUp.frameHdr.Fcnt,&stRadioRxList);
					del_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);
					del_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer2);
					stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].stTxData.len = 0;
					if(stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].classtype == CLASS_A)
					{
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.function = LoRaWANDataDownTimer1Callback;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.data = stFrameDataUp.frameHdr.DevAddr;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.expires = stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].jiffies + LoRaMacParams.ReceiveDelay1;
						add_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);
					}
					else if(stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].classtype == CLASS_C)
					{
						/*stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.function = LoRaWANDataDownClassCTimer1Callback;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.data = stFrameDataUp.frameHdr.DevAddr;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.expires = stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].jiffies + LoRaMacParams.ReceiveDelay1;
						add_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);*/
					}
					else
					{
						printk("unsurported class type\r\n");
					}
					if(stFrameDataUp.frameHdr.Fctrl.Bits.Ack)
					{
					    //stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].stTxData.needack = false;
                        printk("Ack :%d\r\n",stFrameDataUp.frameHdr.DevAddr);
					    //stFrameDataUp.frameHdr.Fctrl.Bits.Ack = false;
						/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
						memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.DevEUI,8);
						stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerSuccess;
						ServerMsgUpListAdd(&stServerMsgUp);*/
					}
					if( ( ( stRadioRxList.stRadioRx.size - 4 ) - appPayloadStartIndex ) > 0 )
					{
						port = stRadioRxList.stRadioRx.payload[appPayloadStartIndex++];
						frameLen = ( stRadioRxList.stRadioRx.size - 4 ) - appPayloadStartIndex;
						if(frameLen > 0)
						{
						    printk("Data :%d\r\n",stFrameDataUp.frameHdr.DevAddr);
							stServerMsgUp.Msg.stData2Server.payload = payloadTx;
							
                            if( port == 0 )
                            {
                                // Only allow frames which do not have fOpts
                                if( stFrameDataUp.frameHdr.Fctrl.Bits.FOptsLen == 0 )
                                {
                                    LoRaMacPayloadDecrypt( stRadioRxList.stRadioRx.payload + appPayloadStartIndex,
                                                           frameLen,
                                                           stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.NwkSKey,
                                                           stFrameDataUp.frameHdr.DevAddr,
                                                           UP_LINK,
                                                           stFrameDataUp.frameHdr.Fcnt,
                                                           stServerMsgUp.Msg.stData2Server.payload );
                                }
                                else
                                {
                                	// Decode Options field MAC commands. Omit the fPort.
                                    //ProcessMacCommands( payload, 8, appPayloadStartIndex - 1, snr );
                                }
                            }
                            else
                            {
                                if( stFrameDataUp.frameHdr.Fctrl.Bits.FOptsLen > 0 )
                                {
                                    // Decode Options field MAC commands. Omit the fPort.
                                    //ProcessMacCommands( payload, 8, appPayloadStartIndex - 1, snr );
                                }
								else
								{
	                                LoRaMacPayloadDecrypt( stRadioRxList.stRadioRx.payload + appPayloadStartIndex,
	                                                       frameLen,
	                                                       stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.AppSKey,
	                                                       stFrameDataUp.frameHdr.DevAddr,
	                                                       UP_LINK,
	                                                       stFrameDataUp.frameHdr.Fcnt,
	                                                       stServerMsgUp.Msg.stData2Server.payload );
								}
                            }
						}
					}
                    stServerMsgUp.enMsgUpFramType = en_MsgUpFramDataReceive;
					memcpy(stServerMsgUp.Msg.stData2Server.AppEUI,stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.AppEUI,8);
					memcpy(stServerMsgUp.Msg.stData2Server.DevEUI,stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.DevEUI,8);
					stServerMsgUp.Msg.stData2Server.DevAddr = stFrameDataUp.frameHdr.DevAddr;
					stServerMsgUp.Msg.stData2Server.Battery = stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].Battery;
					stServerMsgUp.Msg.stData2Server.CtrlBits.AckRequest = (macHdr.Bits.MType == FRAME_TYPE_DATA_CONFIRMED_UP)?true:false;
					stServerMsgUp.Msg.stData2Server.CtrlBits.Ack = stFrameDataUp.frameHdr.Fctrl.Bits.Ack;
					stServerMsgUp.Msg.stData2Server.ClassType = macHdr.Bits.RFU;
					stServerMsgUp.Msg.stData2Server.fPort = port;
					stServerMsgUp.Msg.stData2Server.rssi = stRadioRxList.stRadioRx.rssi;
					stServerMsgUp.Msg.stData2Server.snr = stRadioRxList.stRadioRx.snr;
                    stServerMsgUp.Msg.stData2Server.sn = stFrameDataUp.frameHdr.Fcnt;
					stServerMsgUp.Msg.stData2Server.size = frameLen;
					ServerMsgUpListAdd(&stServerMsgUp);

					break;
				default:
					DEBUG_OUTPUT_INFO("error: unKnown Frame Type!\r\n");
					break;
			}
        }
    }
    return 0;
}

void LoRaWANJoinTimer1WorkQueue(struct work_struct *p_work)
{
    
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer1);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	
	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("no accept ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("accept busy ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.JoinAcceptDelay1 + 50,jiffies))
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("accept delay ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
    mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	//stNodeDatabase[pstMyWork->param].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	//printk("%s,chip = %d\r\n",__func__,stNodeDatabase[pstMyWork->param].chip);
}
#if 0
void LoRaWANJoinTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[pstMyWork->param].chip) == RF_TX_RUNNING)

	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("no accept ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("accept busy ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.JoinAcceptDelay2 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("accept delay ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
    mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	//stNodeDatabase[pstMyWork->param].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
}
#endif
void LoRaWANDataDownTimer1WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	//tasklet_kill(&stNodeDatabase[pstMyWork->param].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer1);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);

	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("no data ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("data busy ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay1 + 50,(unsigned long)jiffies))
	{
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		printk("data delay ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
    #if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownWaitAckTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 3 * HZ;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	}
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
}
#if 0
void LoRaWANDataDownTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	st_ServerMsgUp stServerMsgUp;
	//tasklet_kill(&stNodeDatabase[pstMyWork->param].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[pstMyWork->param].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",pstMyWork->param);		
		/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);*/
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2 + 25,(unsigned long)jiffies))
	{
		//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",pstMyWork->param);
		/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);*/
		return;
	}
    mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	#if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownWaitAckTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 3 * HZ;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	}
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
}

void LoRaWANDataDownWaitAckTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	st_ServerMsgUp stServerMsgUp;
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	
	printk("Nack ,%d\r\n",pstMyWork->param);
	/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
	memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
	stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerNodeNoAck;
	ServerMsgUpListAdd(&stServerMsgUp);*/
}

void LoRaWANDataDownClassCTimer1WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	//tasklet_kill(&stNodeDatabase[pstMyWork->param].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer1);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);

	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		printk("no data ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		printk("data busy ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay1 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		printk("data delay ,1 ,%d\r\n",pstMyWork->param);
		return;
	}
	mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
    #if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 3 * HZ;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	}
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
}
#endif
#if 0
void LoRaWANDataDownClassCTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	st_ServerMsgUp stServerMsgUp;
	//tasklet_kill(&stNodeDatabase[pstMyWork->param].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[pstMyWork->param].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
        stNodeDatabase[pstMyWork->param].stTxData.classcjiffies = jiffies;
        stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
        stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
        stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 1000;
        add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",pstMyWork->param);		
		/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);*/
		return;
	}
	if(time_before(stNodeDatabase[pstMyWork->param].jiffies + LoRaMacParams.ReceiveDelay2 + 100 + 3000,(unsigned long)jiffies))
	{
        stNodeDatabase[pstMyWork->param].stTxData.classcjiffies = jiffies;
        stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
        stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
        stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 1000;
        add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",pstMyWork->param);
		/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);*/
		return;
	}
    mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
    //stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	#if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 3 * HZ;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	}
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
}
#endif
void LoRaWANRadomDataDownClassCTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	st_ServerMsgUp stServerMsgUp;
	//printk("%s, %d\r\n", __func__, __LINE__);
	//tasklet_kill(&stNodeDatabase[pstMyWork->param].tasklet);
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[pstMyWork->param].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[pstMyWork->param].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",pstMyWork->param);
		return;
	}
	//printk("%s, %d\r\n", __func__, __LINE__);
	if(Radio.GetStatus(stNodeDatabase[pstMyWork->param].chip) == RF_TX_RUNNING)
	{
		/*stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",pstMyWork->param);		
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		*/
		
		/*stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 1000;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);*/
		//printk("%s, %d\r\n", __func__, __LINE__);
		return;
	}
	#if 0
	if(time_before(stNodeDatabase[pstMyWork->param].stTxData.classcjiffies + LoRaMacParams.ReceiveDelay2 + 100 + 3000,(unsigned long)jiffies))
	{
	    stNodeDatabase[pstMyWork->param].stTxData.classcjiffies = jiffies;
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 1000;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
		//stNodeDatabase[pstMyWork->param].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",pstMyWork->param);
		/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);*/
		return;
	}
	#endif
	//printk("%s, %d\r\n", __func__, __LINE__);
    mutex_lock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
	Radio.Sleep(stNodeDatabase[pstMyWork->param].chip);
	Radio.SetMaxPayloadLength(stNodeDatabase[pstMyWork->param].chip,MODEM_LORA,0xff);
	Radio.SetTxConfig(stNodeDatabase[pstMyWork->param].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[pstMyWork->param].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[pstMyWork->param].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[pstMyWork->param].chip]]);
	Radio.Send(stNodeDatabase[pstMyWork->param].chip,stNodeDatabase[pstMyWork->param].stTxData.buf,stNodeDatabase[pstMyWork->param].stTxData.len);
    mutex_unlock(&RadioChipMutex[stNodeDatabase[pstMyWork->param].chip]);
    //printk("%s, %d\r\n", __func__, __LINE__);
    //stNodeDatabase[pstMyWork->param].stTxData.len = 0;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	#if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
		stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
		stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 3 * HZ;
		add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	}
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
}
#if 0
void LoRaWANDataDownClassCWaitAckTimer2WorkQueue(struct work_struct *p_work)
{
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
	st_ServerMsgUp stServerMsgUp;
	del_timer(&stNodeDatabase[pstMyWork->param].timer2);
    //stNodeDatabase[pstMyWork->param].stTxData.needack = false;
    stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    #if 0
	if(stNodeDatabase[pstMyWork->param].stTxData.needack)
	{
        stNodeDatabase[pstMyWork->param].stTxData.classcjiffies = jiffies;
        stNodeDatabase[pstMyWork->param].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
        stNodeDatabase[pstMyWork->param].timer2.data = pstMyWork->param;
        stNodeDatabase[pstMyWork->param].timer2.expires = jiffies + 1000;
        add_timer(&stNodeDatabase[pstMyWork->param].timer2);
	    printk("Nack ,%d\r\n",pstMyWork->param);
    }
    else
    {
        stNodeDatabase[pstMyWork->param].stTxData.len = 0;
    }
    #endif
	/*stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
	memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstMyWork->param].stDevNetParameter.DevEUI,8);
	stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerNodeNoAck;
	ServerMsgUpListAdd(&stServerMsgUp);*/
}
#endif
void LoRaWANAddWorkQueue(unsigned long addr,pdotasklet do_work)
{
	//tasklet_kill(&stNodeDatabase[addr].tasklet);
	stNodeDatabase[addr].stMyWork.param = addr;
    INIT_WORK(&(stNodeDatabase[addr].stMyWork.save), do_work);
    queue_work(RadioWorkQueue, &(stNodeDatabase[addr].stMyWork.save));
}
void LoRaWANJoinTimer1Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANJoinTimer1WorkQueue);
	//LoRaWANJoinTimer1Tasklet(index);
}
#if 0
void LoRaWANJoinTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANJoinTimer2WorkQueue);
	//LoRaWANJoinTimer2Tasklet(index);
}
#endif
void LoRaWANJoinAccept(uint32_t addr)
{
	uint8_t acceptbuf[128] = {0};
	LoRaMacHeader_t macHdr;

	printk("%s,%d,%d\r\n",__func__,addr,stNodeDatabase[addr].chip);
	del_timer(&stNodeDatabase[addr].timer1);
	del_timer(&stNodeDatabase[addr].timer2);
	macHdr.Value = 0;
	macHdr.Bits.MType = FRAME_TYPE_JOIN_ACCEPT;
	acceptbuf[0] = macHdr.Value;
    
	memcpy(acceptbuf + 1,stGatewayParameter.AppNonce,3);
	stGatewayParameter.NetID[0] = 0x00;
	stGatewayParameter.NetID[1] = 0x00;
	stGatewayParameter.NetID[2] = 0x00;
	stGatewayParameter.NetID[stRadioCfg_Rx.channel[0] / 8] |= (1 << (stRadioCfg_Rx.channel[0] % 8));
	stGatewayParameter.NetID[stRadioCfg_Rx.channel[1] / 8] |= (1 << (stRadioCfg_Rx.channel[1] % 8));
	memcpy(acceptbuf + 1 + 3,stGatewayParameter.NetID,3);
	*(uint32_t *)(acceptbuf + 1 + 3 + 3) = stNodeInfoToSave[addr].stDevNetParameter.DevAddr;
	acceptbuf[1 + 3 + 3 + 4]= 0;
	acceptbuf[1 + 3 + 3 + 4 + 1] = 0;
	stNodeDatabase[addr].stTxData.len = 1 + 3 + 3 + 4 + 1 + 1 + 4;
	//stNodeDatabase[addr].stTxData.buf = (uint8_t *)kmalloc(1 + 3 + 3 + 4 + 1 + 1 + 4,GFP_KERNEL);
	stNodeDatabase[addr].stTxData.buf[0] = macHdr.Value;
	LoRaMacJoinComputeMic(acceptbuf,13,stGatewayParameter.AppKey,(uint32_t *)(acceptbuf + 1 + 3 + 3 + 4 + 1 + 1));
	*(uint32_t *)(acceptbuf + 1 + 3 + 3 + 4 + 1 + 1) += stNodeInfoToSave[addr].stDevNetParameter.DevNonce;
    LoRaMacJoinDecrypt(acceptbuf + 1,1 + 3 + 3 + 4 + 1 + 1 + 4 - 1,stGatewayParameter.AppKey,stNodeDatabase[addr].stTxData.buf + 1);
	stNodeDatabase[addr].timer1.function = LoRaWANJoinTimer1Callback;
	stNodeDatabase[addr].timer1.data = addr;
	stNodeDatabase[addr].timer1.expires = stNodeDatabase[addr].jiffies + LoRaMacParams.JoinAcceptDelay1;
	add_timer(&stNodeDatabase[addr].timer1);
	stNodeDatabase[addr].sequence_down = 0;
}

void LoRaWANDataDownTimer1Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownTimer1WorkQueue);
	//LoRaWANDataDownTimer1Tasklet(index);
}
#if 0
void LoRaWANDataDownTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownTimer2WorkQueue);
	//LoRaWANDataDownTimer2Tasklet(index);
}

void LoRaWANDataDownWaitAckTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownWaitAckTimer2WorkQueue);
	//LoRaWANDataDownWaitAckTimer2Tasklet(index);
}

void LoRaWANDataDownClassCTimer1Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownClassCTimer1WorkQueue);
	//LoRaWANDataDownClassCTimer1Tasklet(index);
}
#endif
#if 0
void LoRaWANDataDownClassCTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownClassCTimer2WorkQueue);
	//LoRaWANDataDownClassCTimer2Tasklet(index);
}

void LoRaWANDataDownClassCWaitAckTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANDataDownClassCWaitAckTimer2WorkQueue);
	//LoRaWANDataDownClassCWaitAckTimer2Tasklet(index);
}
#endif
void LoRaWANRadomDataDownClassCTimer2Callback( unsigned long index )
{
	LoRaWANAddWorkQueue(index,LoRaWANRadomDataDownClassCTimer2WorkQueue);
	//LoRaWANRadomDataDownClassCTimer2Tasklet(index);
}

