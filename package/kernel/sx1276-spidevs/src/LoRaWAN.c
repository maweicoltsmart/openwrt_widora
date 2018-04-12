#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include "LoRaWAN.h"
#include "proc.h"

LoRaMacParams_t LoRaMacParams;
st_GatewayParameter stGatewayParameter;
static struct task_struct *radio_routin;

typedef void (*pdotasklet)(unsigned long);

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
    LoRaMacParams.JoinAcceptDelay1 = 5 * HZ - 11 - 3;
    LoRaMacParams.JoinAcceptDelay2 = 6 * HZ - 3 - 3;
    LoRaMacParams.ReceiveDelay1 = 1 * HZ - 11 - 3;
    LoRaMacParams.ReceiveDelay2 = 2 * HZ - 3 - 3;
	memcpy(stGatewayParameter.AppKey,stMacCfg.APPKEY,16);
	DEBUG_OUTPUT_DATA((unsigned char *)stGatewayParameter.AppKey,16);
	memcpy(stGatewayParameter.NetID,stMacCfg.NetID,3);
	memcpy(stGatewayParameter.AppNonce,stMacCfg.AppNonce,3);
	ServerMsgInit();
	NodeDatabaseInit();
	RadioInit();
	radio_routin = kthread_create(LoRaWANRxDataProcess, NULL, "LoRaWANRxDataProcess");
    wake_up_process(radio_routin);
}

void LoRaWANRemove(void)
{
	kthread_stop(radio_routin);
	RadioRemove();
	ServerMsgRemove();
	NodeDatabaseRemove();
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
	uint32_t addr = 0;
	
	stRadioRxList.stRadioRx.payload = payloadRx;
    while (true)
    {
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
						if((addr = NodeDatabaseJoin(&stNodeJoinInfo)) < 0)
						{
							printk("%s,%d\r\n",__func__,__LINE__);
						}
						else	// Send join accept fram
						{
							stNodeDatabase[addr].state = enStateJoinning;
							LoRaWANJoinAccept(addr);
						}
					}
					break;
				case FRAME_TYPE_DATA_UNCONFIRMED_UP:
				case FRAME_TYPE_DATA_CONFIRMED_UP:
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
                    printk("%s,%d,%d,%d\r\n",__func__,__LINE__,stFrameDataUp.frameHdr.DevAddr,stRadioRxList.stRadioRx.chip);
					LoRaMacComputeMic( stRadioRxList.stRadioRx.payload, stRadioRxList.stRadioRx.size - LORAMAC_MFR_LEN, stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.NwkSKey, stFrameDataUp.frameHdr.DevAddr, UP_LINK, stFrameDataUp.frameHdr.Fcnt, &micRx );
					if(stFrameDataUp.mic != micRx)
					{
						printk("error: mic! 0x%08X, 0x%08X\r\n",stFrameDataUp.mic,micRx);
						break;
					}
					printk("%s,%d,%d,%d\r\n",__func__,__LINE__,stFrameDataUp.frameHdr.DevAddr,stRadioRxList.stRadioRx.chip);
					NodeDatabaseUpdateParameters(stFrameDataUp.frameHdr.DevAddr,&stRadioRxList);
					del_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);
					del_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer2);
					stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].stTxData.len = 0;
					if(stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].classtype == CLASS_A)
					{
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.function = LoRaWANDataDownTimer1Callback;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.data = stFrameDataUp.frameHdr.DevAddr;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.expires = stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].jiffies + LoRaMacParams.ReceiveDelay1;
						add_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].state = enStateRxWindow1;
					}
					else if(stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].classtype == CLASS_C)
					{
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.function = LoRaWANDataDownClassCTimer1Callback;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.data = stFrameDataUp.frameHdr.DevAddr;
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1.expires = stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].jiffies + LoRaMacParams.ReceiveDelay1;
						add_timer(&stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].timer1);
						stNodeDatabase[stFrameDataUp.frameHdr.DevAddr].state = enStateRxWindow1;
					}
					else
					{
						printk("unsurported class type\r\n");
					}
					if(stFrameDataUp.frameHdr.Fctrl.Bits.Ack)
					{
						stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
						memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[stFrameDataUp.frameHdr.DevAddr].stDevNetParameter.DevEUI,8);
						stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerSuccess;
						ServerMsgUpListAdd(&stServerMsgUp);
					}
					if( ( ( stRadioRxList.stRadioRx.size - 4 ) - appPayloadStartIndex ) > 0 )
					{
						port = stRadioRxList.stRadioRx.payload[appPayloadStartIndex++];
						frameLen = ( stRadioRxList.stRadioRx.size - 4 ) - appPayloadStartIndex;
						if(frameLen > 0)
						{
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
							stServerMsgUp.Msg.stData2Server.size = frameLen;
							ServerMsgUpListAdd(&stServerMsgUp);
						}
					}
					break;
				default:
					DEBUG_OUTPUT_INFO("error: unKnown Frame Type!\r\n");
					break;
			}
        }
    }
    return 0;
}

void LoRaWANJoinTimer1Tasklet(unsigned long index)
{
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer1);
	del_timer(&stNodeDatabase[index].timer2);
	
	if(stNodeDatabase[index].stTxData.len == 0)
	{
		stNodeDatabase[index].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("no accept ,1 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[index].chip) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("accept busy ,1 ,%d\r\n",index);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.JoinAcceptDelay1 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[index].timer2.function = LoRaWANJoinTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.JoinAcceptDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("accept delay ,1 ,%d\r\n",index);
		return;
	}
	Radio.Sleep(stNodeDatabase[index].chip);
	Radio.SetTxConfig(stNodeDatabase[index].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[index].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[index].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[index].chip]]);
	Radio.Send(stNodeDatabase[index].chip,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
}

void LoRaWANJoinTimer2Tasklet(unsigned long index)
{
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[index].chip) == RF_TX_RUNNING)

	if(stNodeDatabase[index].stTxData.len == 0)
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("no accept ,2 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(0) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("accept busy ,2 ,%d\r\n",index);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.JoinAcceptDelay2 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("accept delay ,2 ,%d\r\n",index);
		return;
	}
	Radio.Sleep(0);
	Radio.SetTxConfig(0,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		12,
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(0,stRadioCfg_Tx.freq_tx[25]);
	Radio.Send(0,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
}

void LoRaWANDataDownTimer1Tasklet(unsigned long index)
{
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer1);
	del_timer(&stNodeDatabase[index].timer2);

	if(stNodeDatabase[index].stTxData.len == 0)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("no data ,1 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[index].chip) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("data busy ,1 ,%d\r\n",index);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay1 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("data delay ,1 ,%d\r\n",index);
		return;
	}
	
	Radio.Sleep(stNodeDatabase[index].chip);
	Radio.SetTxConfig(stNodeDatabase[index].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[index].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[index].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[index].chip]]);
	Radio.Send(stNodeDatabase[index].chip,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	if(stNodeDatabase[index].stTxData.needack)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownWaitAckTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + HZ;
		add_timer(&stNodeDatabase[index].timer2);
	}
}

void LoRaWANDataDownTimer2Tasklet(unsigned long index)
{
	st_ServerMsgUp stServerMsgUp;
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[index].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[index].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(0) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",index);		
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",index);
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		return;
	}
	Radio.Sleep(0);
	Radio.SetTxConfig(0,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		12,
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(0,stRadioCfg_Tx.freq_tx[25]);
	Radio.Send(0,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	
	if(stNodeDatabase[index].stTxData.needack)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownWaitAckTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + HZ;
		add_timer(&stNodeDatabase[index].timer2);
	}
}

void LoRaWANDataDownWaitAckTimer2Tasklet(unsigned long index)
{
	st_ServerMsgUp stServerMsgUp;
	del_timer(&stNodeDatabase[index].timer2);
	
	printk("Nack ,%d\r\n",index);
	stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
	memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
	stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerNodeNoAck;
	ServerMsgUpListAdd(&stServerMsgUp);
}

void LoRaWANDataDownClassCTimer1Tasklet(unsigned long index)
{
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer1);
	del_timer(&stNodeDatabase[index].timer2);

	if(stNodeDatabase[index].stTxData.len == 0)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("no data ,1 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(stNodeDatabase[index].chip) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("data busy ,1 ,%d\r\n",index);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay1 + 25,(unsigned long)jiffies))
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2;
		add_timer(&stNodeDatabase[index].timer2);
		printk("data delay ,1 ,%d\r\n",index);
		return;
	}
	
	Radio.Sleep(stNodeDatabase[index].chip);
	Radio.SetTxConfig(stNodeDatabase[index].chip,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		stRadioCfg_Tx.datarate[stNodeDatabase[index].chip],
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(stNodeDatabase[index].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[stNodeDatabase[index].chip]]);
	Radio.Send(stNodeDatabase[index].chip,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	if(stNodeDatabase[index].stTxData.needack)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + HZ;
		add_timer(&stNodeDatabase[index].timer2);
	}
}

void LoRaWANDataDownClassCTimer2Tasklet(unsigned long index)
{
	st_ServerMsgUp stServerMsgUp;
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[index].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[index].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(0) == RF_TX_RUNNING)
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",index);		
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].jiffies + LoRaMacParams.ReceiveDelay2 + 100 + 3000,(unsigned long)jiffies))
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",index);
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		return;
	}
	Radio.Sleep(0);
	Radio.SetTxConfig(0,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		12,
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(0,stRadioCfg_Tx.freq_tx[25]);
	Radio.Send(0,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	
	if(stNodeDatabase[index].stTxData.needack)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + HZ;
		add_timer(&stNodeDatabase[index].timer2);
	}
}
void LoRaWANRadomDataDownClassCTimer2Tasklet(unsigned long index)
{
	st_ServerMsgUp stServerMsgUp;
	//tasklet_kill(&stNodeDatabase[index].tasklet);
	del_timer(&stNodeDatabase[index].timer2);
	//if(Radio.GetStatus(stNodeInfoToSave[index].chip) == RF_TX_RUNNING)
	if(stNodeDatabase[index].stTxData.len == 0)
	{
		printk("no data ,2 ,%d\r\n",index);
		return;
	}
	if(Radio.GetStatus(0) == RF_TX_RUNNING)
	{
		/*stNodeDatabase[index].stTxData.len = 0;
		printk("data busy ,2 ,%d\r\n",index);		
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		*/
		
		stNodeDatabase[index].timer2.function = LoRaWANRadomDataDownClassCTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + 200;
		add_timer(&stNodeDatabase[index].timer2);
		stNodeDatabase[index].state = enStateRxWindow1;
		return;
	}
	if(time_before((unsigned long)stNodeDatabase[index].stTxData.classcjiffies + LoRaMacParams.ReceiveDelay2 + 100 + 3000,(unsigned long)jiffies))
	{
		stNodeDatabase[index].stTxData.len = 0;
		printk("data delay ,2 ,%d\r\n",index);
		stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
		memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
		stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
		ServerMsgUpListAdd(&stServerMsgUp);
		return;
	}
	Radio.Sleep(0);
	Radio.SetTxConfig(0,
		stRadioCfg_Tx.modem,
		stRadioCfg_Tx.power,
		stRadioCfg_Tx.fdev,
		stRadioCfg_Tx.bandwidth,
		12,
		stRadioCfg_Tx.coderate,
		stRadioCfg_Tx.preambleLen,
		stRadioCfg_Tx.fixLen,
		stRadioCfg_Tx.crcOn,
		stRadioCfg_Tx.freqHopOn,
		stRadioCfg_Tx.hopPeriod,
		stRadioCfg_Tx.iqInverted,
		stRadioCfg_Tx.timeout);
	Radio.SetChannel(0,stRadioCfg_Tx.freq_tx[25]);
	Radio.Send(0,stNodeDatabase[index].stTxData.buf,stNodeDatabase[index].stTxData.len);
	stNodeDatabase[index].stTxData.len = 0;
	stNodeDatabase[index].state = enStateJoined;
	DEBUG_OUTPUT_INFO("%s\r\n",__func__);
	
	if(stNodeDatabase[index].stTxData.needack)
	{
		stNodeDatabase[index].timer2.function = LoRaWANDataDownClassCWaitAckTimer2Callback;
		stNodeDatabase[index].timer2.data = index;
		stNodeDatabase[index].timer2.expires = jiffies + HZ;
		add_timer(&stNodeDatabase[index].timer2);
	}
}

void LoRaWANDataDownClassCWaitAckTimer2Tasklet(unsigned long index)
{
	st_ServerMsgUp stServerMsgUp;
	del_timer(&stNodeDatabase[index].timer2);
	
	printk("Nack ,%d\r\n",index);
	stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
	memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8);
	stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerNodeNoAck;
	ServerMsgUpListAdd(&stServerMsgUp);
}

void LoRaWANAddTaskLet(unsigned long addr,pdotasklet dotasklet)
{
	//tasklet_kill(&stNodeDatabase[addr].tasklet);
	tasklet_init(&stNodeDatabase[addr].tasklet,  dotasklet, addr);  
	tasklet_schedule(&stNodeDatabase[addr].tasklet);
}
void LoRaWANJoinTimer1Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANJoinTimer1Tasklet);
	LoRaWANJoinTimer1Tasklet(index);
}

void LoRaWANJoinTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANJoinTimer2Tasklet);
	LoRaWANJoinTimer2Tasklet(index);
}

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
	stNodeDatabase[addr].state = enStateRxWindow1;
}

void LoRaWANDataDownTimer1Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownTimer1Tasklet);
	LoRaWANDataDownTimer1Tasklet(index);
}

void LoRaWANDataDownTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownTimer2Tasklet);
	LoRaWANDataDownTimer2Tasklet(index);
}

void LoRaWANDataDownWaitAckTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownWaitAckTimer2Tasklet);
	LoRaWANDataDownWaitAckTimer2Tasklet(index);
}

void LoRaWANDataDownClassCTimer1Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownClassCTimer1Tasklet);
	LoRaWANDataDownClassCTimer1Tasklet(index);
}

void LoRaWANDataDownClassCTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownClassCTimer2Tasklet);
	LoRaWANDataDownClassCTimer2Tasklet(index);
}

void LoRaWANDataDownClassCWaitAckTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANDataDownClassCWaitAckTimer2Tasklet);
	LoRaWANDataDownClassCWaitAckTimer2Tasklet(index);
}

void LoRaWANRadomDataDownClassCTimer2Callback( unsigned long index )
{
	//LoRaWANAddTaskLet(index,LoRaWANRadomDataDownClassCTimer2Tasklet);
	LoRaWANRadomDataDownClassCTimer2Tasklet(index);
}

