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
}

void OnMacRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    DEBUG_OUTPUT_EVENT(chip, EV_RXCOMPLETE);
    Radio.Sleep( chip);
    RadioRxMsgListAdd( chip,payload,size,rssi,snr );
    Radio.Rx( chip,0 );
    rx_done = true;
    wake_up(&lora_wait);
}

void OnMacTxDone( int chip )
{
	DEBUG_OUTPUT_EVENT(chip,EV_TXCOMPLETE);
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
    //struct lora_tx_data *p2;
    LoRaMacHeader_t macHdr;
    //LoRaMacFrameCtrl_t fCtrl;
    node_join_info_t node_join_info;
    uint8_t *pkg;
	uint32_t micRX,micTX;
	printk("%s,%d\r\n",__func__,__LINE__);
    while (true)
    {
    	if( kthread_should_stop())
        {
			return -1;
        }
    	if(RadioRxMsgListGet(radiorxbuffer) < 0)
    	{
    		rx_done = false;
        	//wait_event_interruptible(lora_wait,rx_done);
			wait_event_interruptible_timeout(lora_wait, rx_done, HZ);
    	}
        else
        {
            p1 = (struct lora_rx_data*)radiorxbuffer;
            pkg = (uint8_t*)&radiorxbuffer[sizeof(struct lora_rx_data)];
			DEBUG_OUTPUT_DATA(pkg,p1->size);
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
                    node_join_info.DevNonce = *(uint16_t*)&pkg[17];
                    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
                    index = database_node_join(&node_join_info);
                    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
                    //p2 = (struct lora_tx_data *)&radiotxbuffer[1];
                    pkg = (uint8_t*)&radiotxbuffer[1][0];
                    macHdr.Bits.MType = FRAME_TYPE_JOIN_ACCEPT;
                    pkg[0] = macHdr.Value;
                    memcpy(&pkg[1],gateway_pragma.AppNonce,3); // AppNonce
                    memcpy(&pkg[1 + 3],gateway_pragma.NetID,3); // NetID
                    memcpy(&pkg[1 + 3 + 3],nodebase_node_pragma[index].DevAddr,4);  // DevAddr
                    pkg[1 + 3 + 3 + 4] = 0; //DLSettings
                    pkg[1 + 3 + 3 + 4 + 1] = 0; //RxDelay
                    LoRaMacJoinComputeMic(radiotxbuffer[1],13,gateway_pragma.APPKEY,(uint32_t*)&pkg[1 + 3 + 3 + 4 + 1 + 1]);   // mic
					debug_output_data(pkg,17);
					LoRaMacJoinDecrypt(&pkg[1],12 + 4,gateway_pragma.APPKEY,&radiotxbuffer[0][1]);
                    pkg = (uint8_t*)&radiotxbuffer[0][0];
                    pkg[0]= macHdr.Value;
                    //p2 = (struct lora_tx_data *)&radiotxbuffer[0];
                    //p2->chip = p1->chip;
                    //p2->size = 17;
					update_node_info(index,p1->chip);
					RadioTxMsgListAdd(index,pkg,17);
                    //p2->freq = CN470_FIRST_RX1_CHANNEL + ( p2->chip ) * CN470_STEPWIDTH_RX1_CHANNEL;
                    //p2->freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
                    //Radio.Send(radiotxbuffer[1],17);
                    break;
                case FRAME_TYPE_DATA_UNCONFIRMED_UP:
					DEBUG_OUTPUT_EVENT(p1->chip,EV_DATA_UNCONFIRMED_UP);
                    break;
                case FRAME_TYPE_DATA_CONFIRMED_UP:
					DEBUG_OUTPUT_EVENT(p1->chip,EV_DATA_CONFIRMED_UP);
                    break;
                case FRAME_TYPE_PROPRIETARY:
                    break;
                default:

                    break;
            }
            msleep(10000);
        }
    }
    return 0;
}
