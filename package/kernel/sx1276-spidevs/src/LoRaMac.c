#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include "LoRaMac.h"
#include "radiomsg.h"
#include "radio.h"
#include "proc.h"
#include "nodedatabase.h"
#include "LoRaMacCrypto.h"
#include "debug.h"

static DECLARE_WAIT_QUEUE_HEAD(lora_wait);
LoRaMacParams_t LoRaMacParams;
static bool rx_done = false;
void LoRaMacInit(void)
{
    LoRaMacParams.JoinAcceptDelay1 = 5 * HZ;
    LoRaMacParams.JoinAcceptDelay2 = 6 * HZ;
    LoRaMacParams.ReceiveDelay1 = 1 * HZ;
    LoRaMacParams.ReceiveDelay2 = 2 * HZ;
}

void OnMacRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( chip);
    RadioRxMsgListAdd( chip,payload,size,rssi,snr );
	DEBUG_OUTPUT_EVENT(chip, EV_RXCOMPLETE);
	DEBUG_OUTPUT_DATA(payload,size);
    Radio.Rx( chip,0 );
    rx_done = true;
    wake_up(&lora_wait);
}

void OnMacTxDone( int chip )
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

void OnMacTxTimeout( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,0 );
    //State = TX_TIMEOUT;
}

void OnMacRxTimeout( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,0 );
    //State = RX_TIMEOUT;
}

void OnMacRxError( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,0 );
    //State = RX_ERROR;
}

int Radio_routin(void *data){
    int index;
    unsigned char radiorxbuffer[300];
    unsigned char radiotxbuffer[2][300];
    //int len;
    struct lora_rx_data *p1;
    uint8_t *LoRaMacRxPayload = radiotxbuffer[0];
    uint8_t *payload;
    //struct lora_tx_data *p2;
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t fCtrl;
    bool isMicOk = false;
    uint8_t pktHeaderLen = 0;
    uint32_t address = 0;
    uint8_t appPayloadStartIndex = 0;
    uint8_t port = 0xFF;
    uint8_t frameLen = 0;
    uint32_t mic = 0;
    uint32_t micRx = 0;
    uint16_t sequenceCounter = 0;
    uint16_t sequenceCounterPrev = 0;
    uint16_t sequenceCounterDiff = 0;
    uint32_t downLinkCounter = 0;
    //LoRaMacFrameCtrl_t fCtrl;
    node_join_info_t node_join_info;
    uint8_t *pkg;
    uint32_t micRX,micTX;
    //printk("%s,%d\r\n",__func__,__LINE__);
    while (true)
    {
        if( kthread_should_stop())
        {
            return -1;
        }
        memset(radiorxbuffer,0,300);
        if(RadioRxMsgListGet(radiorxbuffer) < 0)
        {
            rx_done = false;
            //wait_event_interruptible(lora_wait,rx_done);
            wait_event_interruptible_timeout(lora_wait, rx_done, HZ / 5);
        }
        else
        {
            p1 = (struct lora_rx_data*)radiorxbuffer;
            pkg = (uint8_t*)&radiorxbuffer[sizeof(struct lora_rx_data)];
            //DEBUG_OUTPUT_DATA(pkg,p1->size);
            macHdr.Value = pkg[0];
            switch( macHdr.Bits.MType )
            {
                case FRAME_TYPE_JOIN_REQ:
                    if(p1->size < 19 + 4)
                    {
                        continue;
                    }
                    LoRaMacJoinComputeMic( (const uint8_t*)pkg, 19, gateway_pragma.APPKEY, &micRX );
                    if(micRX != *(uint32_t *)(&pkg[19]))
                    {
                        DEBUG_OUTPUT_DATA((uint8_t*)&micRX,4);
                        DEBUG_OUTPUT_DATA(&pkg[19],4);
                        continue;
                    }
                    DEBUG_OUTPUT_EVENT(p1->chip,EV_JOIN_REQ);
                    memcpy(node_join_info.APPEUI,pkg + 1,8);
                    memcpy(node_join_info.DevEUI,pkg + 9,8);
                    node_join_info.chip = p1->chip;
                    node_join_info.DevNonce = pkg[17];
                    node_join_info.DevNonce |= pkg[18] << 8;
                    node_join_info.snr = p1->snr;
                    node_join_info.rssi = p1->rssi;
                    node_join_info.jiffies = p1->jiffies;
                    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
                    index = node_database_join(&node_join_info);
                    if(index < 0)
                    {
                        break;
                    }
                    break;
                case FRAME_TYPE_DATA_UNCONFIRMED_UP:
                    //DEBUG_OUTPUT_EVENT(p1->chip,EV_DATA_UNCONFIRMED_UP);
                    //break;
                case FRAME_TYPE_DATA_CONFIRMED_UP:
                    sequenceCounter = 0;
                    pktHeaderLen = 0;
                    micRx = 0;
                    mic = 0;
                    isMicOk = false;
                    payload = pkg;
                    macHdr.Value = payload[pktHeaderLen++];

                    address = payload[pktHeaderLen++];
                    address |= ( (uint32_t)payload[pktHeaderLen++] << 8 );
                    address |= ( (uint32_t)payload[pktHeaderLen++] << 16 );
                    address |= ( (uint32_t)payload[pktHeaderLen++] << 24 );
                    if(!node_verify_net_addr(address))
                    {
                        break;
                    }

                    downLinkCounter = ( uint16_t )nodebase_node_pragma[address].sequenceCounter_Up;
                    fCtrl.Value = payload[pktHeaderLen++];
                    //LMIC.txrxFlags |= fCtrl.Bits.Ack ? TXRX_ACK : TXRX_NACK;
                    sequenceCounter = ( uint16_t )payload[pktHeaderLen++];
                    sequenceCounter |= ( uint16_t )payload[pktHeaderLen++] << 8;
                    //nodebase_node_pragma[address].sequenceCounter_Up = ( uint16_t )payload[pktHeaderLen++];
                    //nodebase_node_pragma[address].sequenceCounter_Up |= ( uint16_t )payload[pktHeaderLen++] << 8;

                    appPayloadStartIndex = 8 + fCtrl.Bits.FOptsLen;

                    micRx |= ( uint32_t )payload[p1->size - LORAMAC_MFR_LEN];
                    micRx |= ( ( uint32_t )payload[p1->size - LORAMAC_MFR_LEN + 1] << 8 );
                    micRx |= ( ( uint32_t )payload[p1->size - LORAMAC_MFR_LEN + 2] << 16 );
                    micRx |= ( ( uint32_t )payload[p1->size - LORAMAC_MFR_LEN + 3] << 24 );

                    sequenceCounterPrev = ( uint16_t )downLinkCounter;
                    sequenceCounterDiff = ( sequenceCounter - sequenceCounterPrev );

                    if( sequenceCounterDiff < ( 1 << 15 ) )
                    {
                        downLinkCounter += sequenceCounterDiff;
                        LoRaMacComputeMic( payload, p1->size - LORAMAC_MFR_LEN, nodebase_node_pragma[address].NwkSKey, address, UP_LINK, sequenceCounter, &mic );
                        //printk("%08X\r\n%08X\r\n%08X\r\n%04X\r\n%08X\r\n",micRx,mic,nodebase_node_pragma[address].NwkSKey,nodebase_node_pragma[address].DevNonce,sequenceCounter);
                        if( micRx == mic )
                        {
                            isMicOk = true;
                        }
                    }
                    else
                    {
                        // check for sequence roll-over
                        uint32_t  downLinkCounterTmp = downLinkCounter + 0x10000 + ( int16_t )sequenceCounterDiff;
                        LoRaMacComputeMic( payload, p1->size - LORAMAC_MFR_LEN, nodebase_node_pragma[address].NwkSKey, address, UP_LINK, sequenceCounter, &mic );
                        //printk("%08X\r\n%08X\r\n%08X\r\n%04X\r\n",micRx,mic,nodebase_node_pragma[address].NwkSKey,nodebase_node_pragma[address].DevNonce);
                        if( micRx == mic )
                        {
                            isMicOk = true;
                            downLinkCounter = downLinkCounterTmp;
                        }
                    }

                    if( isMicOk == true )
                    {
                        node_update_info(index,p1);
                        node_time_start(address);
                    	switch(macHdr.Bits.MType)
                    	{
                    		case FRAME_TYPE_DATA_CONFIRMED_UP:
                    			nodebase_node_pragma[address].is_ack_req = true;
								DEBUG_OUTPUT_EVENT(p1->chip,EV_DATA_CONFIRMED_UP);
								break;
							case FRAME_TYPE_DATA_UNCONFIRMED_UP:
								nodebase_node_pragma[address].is_ack_req = false;
								DEBUG_OUTPUT_EVENT(p1->chip,EV_DATA_UNCONFIRMED_UP);
								break;
							default:
								break;
                    	}
                        if(fCtrl.Bits.Ack)
                        {
                            node_delete_repeat_buf(address);
                            node_timer_stop(address);
                        }
                        nodebase_node_pragma[address].sequenceCounter_Up = downLinkCounter;

                        // Process payload and MAC commands
                        if( ( ( p1->size - 4 ) - appPayloadStartIndex ) > 0 )
                        {
                            port = payload[appPayloadStartIndex++];
                            /*
                            if( port < 0 ) {
                                LMIC.txrxFlags |= TXRX_NOPORT;
                            } else {
                                LMIC.txrxFlags |= TXRX_PORT;
                            }*/
                            frameLen = ( p1->size - 4 ) - appPayloadStartIndex;

                            //McpsIndication.Port = port;
                            //printk("%02X, %04X, %02X, %02X, %08X\r\n",port,sequenceCounter, fCtrl.Value,frameLen,address);
                            memset(LoRaMacRxPayload,0,300);
                            if( port == 0 )
                            {
                                // Only allow frames which do not have fOpts
                                if( fCtrl.Bits.FOptsLen == 0 )
                                {
                                    LoRaMacPayloadDecrypt( payload + appPayloadStartIndex,
                                                           frameLen,
                                                           nodebase_node_pragma[address].NwkSKey,
                                                           address,
                                                           UP_LINK,
                                                           sequenceCounter,
                                                           LoRaMacRxPayload );

                                    hexdump(LoRaMacRxPayload,frameLen);
                                    Radio2ServerMsgListAdd(address,port,LoRaMacRxPayload,frameLen);
                                    // Decode frame payload MAC commands
                                    //ProcessMacCommands( LoRaMacRxPayload, 0, frameLen, snr );
                                }
                                else
                                {
                                    //skipIndication = true;
                                }
                            }
                            else
                            {
                                if( fCtrl.Bits.FOptsLen > 0 )
                                {
                                    // Decode Options field MAC commands. Omit the fPort.
                                    //ProcessMacCommands( payload, 8, appPayloadStartIndex - 1, snr );
                                }

                                LoRaMacPayloadDecrypt( payload + appPayloadStartIndex,
                                                       frameLen,
                                                       nodebase_node_pragma[address].AppSKey,
                                                       address,
                                                       UP_LINK,
                                                       sequenceCounter,
                                                       LoRaMacRxPayload );
                                //hexdump(nodebase_node_pragma[address].AppSKey,16);
                                //hexdump(payload + appPayloadStartIndex,frameLen);
                                hexdump(LoRaMacRxPayload,frameLen);
                                Radio2ServerMsgListAdd(address,port,LoRaMacRxPayload,frameLen);
                                /*if( skipIndication == false )
                                {
                                    McpsIndication.Buffer = LoRaMacRxPayload;
                                    McpsIndication.BufferSize = frameLen;
                                    McpsIndication.RxData = true;
                                }*/
                            }
                        }
                        else
                        {
                            if( fCtrl.Bits.FOptsLen > 0 )
                            {
                                // Decode Options field MAC commands
                                //ProcessMacCommands( payload, 8, appPayloadStartIndex, snr );
                            }
                        }

                        /*if( skipIndication == false )
                        {
                            // Check if the frame is an acknowledgement
                            if( fCtrl.Bits.Ack == 1 )
                            {
                                McpsConfirm.AckReceived = true;
                                McpsIndication.AckReceived = true;

                                // Stop the AckTimeout timer as no more retransmissions
                                // are needed.
                                TimerStop( &AckTimeoutTimer );
                            }
                            else
                            {
                                McpsConfirm.AckReceived = false;

                                if( AckTimeoutRetriesCounter > AckTimeoutRetries )
                                {
                                    // Stop the AckTimeout timer as no more retransmissions
                                    // are needed.
                                    TimerStop( &AckTimeoutTimer );
                                }
                            }
                        }
                        // Provide always an indication, skip the callback to the user application,
                        // in case of a confirmed downlink retransmission.
                        LoRaMacFlags.Bits.McpsInd = 1;
                        LoRaMacFlags.Bits.McpsIndSkip = skipIndication;*/
                    }
                    else
                    {
                        /*McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_MIC_FAIL;

                        PrepareRxDoneAbort( );
                        return;*/
                    }
                break;
                case FRAME_TYPE_PROPRIETARY:
                    {
                        /*memcpy1( LoRaMacRxPayload, &payload[pktHeaderLen], size );

                        McpsIndication.McpsIndication = MCPS_PROPRIETARY;
                        McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_OK;
                        McpsIndication.Buffer = LoRaMacRxPayload;
                        McpsIndication.BufferSize = size - pktHeaderLen;

                        LoRaMacFlags.Bits.McpsInd = 1;*/
                        break;
                    }
                default:
                    /*
                    McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                    PrepareRxDoneAbort( );
                    */
                    break;
            }
            /*onEvent(EV_TXCOMPLETE);
            //LoRaMacFlags.Bits.MacDone = 1;
            // Trig OnMacCheckTimerEvent call as soon as possible
            //TimerSetValue( &MacStateCheckTimer, 1 );
            //TimerStart( &MacStateCheckTimer );
            //msleep(10000);
            */
        }
    }
    return 0;
}
