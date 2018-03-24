#include "utilities.h"
#include "unistd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "pthread.h"
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <json-c/json.h>
#include "typedef.h"
#include "nodedatabase.h"
#include "LoRaDevOps.h"
#include "RegionCN470.h"
#include "LoRaMac.h"
#include "sx1276.h"
#include "Region.h"
#include "GatewayPragma.h"
#include "Server.h"

int fd_proc_cfg_rx,fd_proc_cfg_tx,fd_proc_cfg_mac;
int fd_cdev = 0;


static uint8_t *LoRaMacAppKey;
LoRaMacParams_t LoRaMacParams;
LoRaMacParams_t LoRaMacParamsDefaults;

void LoRaMacInit(void)
{
    int chip = 0;
    int i;
    st_RadioCfg stRadioCfg;
	st_MacCfg stMacCfg;
    //const uint8_t symbpreamble[12] = {0,1,2,3,4,5,6,24,14,9,};
    GetGatewayPragma();
open_dev:
    fd_proc_cfg_rx = open("/proc/lora_procfs/lora_cfg_rx",O_RDWR);
    if (fd_proc_cfg_rx < 0)
    {
        //perror("lora_proc_cfg_rx");
        //printf("open lora_proc_cfg_rx error\r\n");
        sleep(1);
        goto open_dev;
    }
    fd_proc_cfg_tx = open("/proc/lora_procfs/lora_cfg_tx",O_RDWR);
    if (fd_proc_cfg_tx < 0)
    {
        //perror("lora_proc_cfg_tx");
        //printf("open lora_proc_cfg_tx error\r\n");
        sleep(1);
        goto open_dev;
    }

	
    fd_proc_cfg_mac = open("/proc/lora_procfs/lora_cfg_mac",O_RDWR);
    if (fd_proc_cfg_mac < 0)
    {
        //perror("lora_proc_cfg_tx");
        //printf("open lora_proc_cfg_tx error\r\n");
        sleep(1);
        goto open_dev;
    }

    LoRaMacParamsDefaults.ChannelsTxPower = CN470_DEFAULT_TX_POWER;
    LoRaMacParamsDefaults.ChannelsDatarate = CN470_DEFAULT_DATARATE;
    LoRaMacParamsDefaults.MaxRxWindow = CN470_MAX_RX_WINDOW;
    LoRaMacParamsDefaults.ReceiveDelay1 = CN470_RECEIVE_DELAY1;
    LoRaMacParamsDefaults.ReceiveDelay2 = CN470_RECEIVE_DELAY2;
    LoRaMacParamsDefaults.JoinAcceptDelay1 = CN470_JOIN_ACCEPT_DELAY1;
    LoRaMacParamsDefaults.JoinAcceptDelay2 = CN470_JOIN_ACCEPT_DELAY2;
    LoRaMacParamsDefaults.Rx1DrOffset = CN470_DEFAULT_RX1_DR_OFFSET;
    LoRaMacParamsDefaults.Rx2Channel.Frequency = CN470_RX_WND_2_FREQ;
    LoRaMacParamsDefaults.Rx2Channel.Datarate = CN470_RX_WND_2_DR;
    //LoRaMacParamsDefaults.UplinkDwellTime = phyParam.Value;
    //LoRaMacParamsDefaults.DownlinkDwellTime = phyParam.Value;
    LoRaMacParamsDefaults.MaxEirp = CN470_DEFAULT_MAX_EIRP;
    LoRaMacParamsDefaults.AntennaGain = CN470_DEFAULT_ANTENNA_GAIN;
    // Init parameters which are not set in function ResetMacParameters
    LoRaMacParamsDefaults.ChannelsNbRep = 1;
    LoRaMacParamsDefaults.SystemMaxRxError = 10;
    LoRaMacParamsDefaults.MinRxSymbols = 6;

    LoRaMacParams.SystemMaxRxError = LoRaMacParamsDefaults.SystemMaxRxError;
    LoRaMacParams.MinRxSymbols = LoRaMacParamsDefaults.MinRxSymbols;
    LoRaMacParams.MaxRxWindow = LoRaMacParamsDefaults.MaxRxWindow;
    LoRaMacParams.ReceiveDelay1 = LoRaMacParamsDefaults.ReceiveDelay1;
    LoRaMacParams.ReceiveDelay2 = LoRaMacParamsDefaults.ReceiveDelay2;
    LoRaMacParams.JoinAcceptDelay1 = LoRaMacParamsDefaults.JoinAcceptDelay1;
    LoRaMacParams.JoinAcceptDelay2 = LoRaMacParamsDefaults.JoinAcceptDelay2;
    LoRaMacParams.ChannelsNbRep = LoRaMacParamsDefaults.ChannelsNbRep;

    for( i = 0; i < CN470_MAX_NB_CHANNELS; i++ )
    {
        stRadioCfg.freq_rx[i] = ( uint32_t )( ( double )(470300000 + i * 200000) / ( double )FREQ_STEP );
    }
    for( i = 0; i < 48; i++ )
    {
        stRadioCfg.freq_tx[i] = ( uint32_t )( ( double )(CN470_FIRST_RX1_CHANNEL + ( i ) * CN470_STEPWIDTH_RX1_CHANNEL) / ( double )FREQ_STEP );
    }
    stRadioCfg.dr_range = (DR_5 << 16) | DR_0;
    stRadioCfg.modem = MODEM_LORA;
    stRadioCfg.power = 20;
    stRadioCfg.fdev = 0;
    stRadioCfg.bandwidth = 0;
    stRadioCfg.datarate[0] = gateway_pragma.radio[0].datarate;
    stRadioCfg.datarate[1] = gateway_pragma.radio[1].datarate;
    stRadioCfg.datarate[2] = gateway_pragma.radio[2].datarate;
    stRadioCfg.channel[0] = gateway_pragma.radio[0].channel;
    stRadioCfg.channel[1] = gateway_pragma.radio[1].channel;
    stRadioCfg.channel[2] = gateway_pragma.radio[2].channel;
    stRadioCfg.coderate = 1;
    stRadioCfg.preambleLen = 8;
    stRadioCfg.fixLen = false;
    stRadioCfg.crcOn = true;
    stRadioCfg.freqHopOn = false;
    stRadioCfg.hopPeriod = 0;
    stRadioCfg.iqInverted = false;
    stRadioCfg.timeout = 3000;
    stRadioCfg.bandwidthAfc = 0;
    stRadioCfg.symbTimeout = 24;
    stRadioCfg.payloadLen = 0;
    stRadioCfg.rxContinuous = true;
    stRadioCfg.isPublic = gateway_pragma.NetType;

    write(fd_proc_cfg_rx,&stRadioCfg,sizeof(st_RadioCfg));

    stRadioCfg.dr_range = (DR_5 << 16) | DR_0;
    stRadioCfg.modem = MODEM_LORA;
    stRadioCfg.power = 20;
    stRadioCfg.fdev = 0;
    stRadioCfg.bandwidth = 0;
    stRadioCfg.datarate[0] = gateway_pragma.radio[0].datarate;
    stRadioCfg.datarate[1] = gateway_pragma.radio[1].datarate;
    stRadioCfg.datarate[2] = gateway_pragma.radio[2].datarate;
    stRadioCfg.channel[0] = gateway_pragma.radio[0].channel % 48;
    stRadioCfg.channel[1] = gateway_pragma.radio[1].channel % 48;
    stRadioCfg.channel[2] = gateway_pragma.radio[2].channel % 48;
    stRadioCfg.coderate = 1;
    stRadioCfg.preambleLen = 8;
    stRadioCfg.fixLen = false;
    stRadioCfg.crcOn = false;
    stRadioCfg.freqHopOn = 0;
    stRadioCfg.hopPeriod = 0;
    stRadioCfg.iqInverted = true;
    stRadioCfg.timeout = 3000;
    stRadioCfg.bandwidthAfc = 0;
    stRadioCfg.symbTimeout = 24;
    stRadioCfg.payloadLen = 0;
    stRadioCfg.rxContinuous = true;
    stRadioCfg.isPublic = gateway_pragma.NetType;

    write(fd_proc_cfg_tx,&stRadioCfg,sizeof(st_RadioCfg));

    memcpy(stMacCfg.APPKEY,gateway_pragma.APPKEY,16);
	memcpy(stMacCfg.NetID,gateway_pragma.NetID,3);
	
    write(fd_proc_cfg_mac,&stMacCfg,sizeof(st_MacCfg));

    sleep(1);
    fd_cdev = open("/dev/lora_radio",O_RDWR | O_NONBLOCK);
    if (fd_cdev < 0)
    {
        sleep(1);
        goto open_dev;
    }
}

void *Radio_routin(void *param){
    uint8_t stringformat[256 * 2];
    int msgid = -1;
    struct msg_st data;
    long int msgtype = 0;
	int len;
    struct json_object *pragma = NULL;
    //lora_server_up_data_type *dataup;
    uint8_t readbuffer[256 + sizeof(st_ServerMsgUp)];
	pst_ServerMsgUp pstServerMsgUp = (pst_ServerMsgUp)readbuffer;
	data.msg_type = 1;
    creat_msg_q:
    //建立消息队列
    while((msgid = msgget((key_t)1234, 0666 | IPC_CREAT) == -1))
    {
        printf("msgget failed with error: %d\r\n", errno);
        sleep(1);
    }
    while(1)
    {
        if((len = read(fd_cdev,readbuffer,sizeof(readbuffer))) > 0)
        {
        	pragma = json_object_new_object();
        	if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramDataReceive)
        	{
        		pstServerMsgUp->Msg.stData2Server.payload = &readbuffer[sizeof(st_ServerMsgUp)];
        		json_object_object_add(pragma,"FrameType",json_object_new_string("UpData"));
				switch(pstServerMsgUp->Msg.stData2Server.ClassType)
				{
					case 0:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class A"));
						break;
					case 1:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class B"));
						break;
					case 2:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class C"));
						break;
				}
				memset(stringformat,0,256 * 2);
            	Hex2Str(pstServerMsgUp->Msg.stData2Server.DevEUI,stringformat,8);
            	json_object_object_add(pragma,"DevEUI",json_object_new_string(stringformat));
            	memset(stringformat,0,256 * 2);
            	Hex2Str(pstServerMsgUp->Msg.stData2Server.AppEUI,stringformat,8);
	            json_object_object_add(pragma,"NetAddr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.DevAddr));
				json_object_object_add(pragma,"AppEUI",json_object_new_string(stringformat));
	            json_object_object_add(pragma,"Port",json_object_new_int(pstServerMsgUp->Msg.stData2Server.fPort));
	            json_object_object_add(pragma,"ConfirmRequest",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest));
				//json_object_object_add(pragma,"Confirm",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack));
	            json_object_object_add(pragma,"Battery",json_object_new_int(pstServerMsgUp->Msg.stData2Server.Battery));
	            json_object_object_add(pragma,"Rssi",json_object_new_int(pstServerMsgUp->Msg.stData2Server.rssi));
	            json_object_object_add(pragma,"Snr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.snr));
	            memset(stringformat,0,256 * 2);
	            Hex2Str(pstServerMsgUp->Msg.stData2Server.payload,stringformat,pstServerMsgUp->Msg.stData2Server.size);
	            json_object_object_add(pragma,"Data",json_object_new_string(stringformat)); /* data that encoded into Base64 */
        	}
			else if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramConfirm)
			{
				json_object_object_add(pragma,"FrameType",json_object_new_string("UpConfirm"));
				switch(pstServerMsgUp->Msg.stConfirm2Server.ClassType)
				{
					case 0:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class A"));
						break;
					case 1:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class B"));
						break;
					case 2:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class C"));
						break;
				}
            	memset(stringformat,0,256 * 2);
            	Hex2Str(pstServerMsgUp->Msg.stConfirm2Server.DevEUI,stringformat,8);
				json_object_object_add(pragma,"DevEUI",json_object_new_string(stringformat));
	            json_object_object_add(pragma,"NetAddr",json_object_new_int(pstServerMsgUp->Msg.stConfirm2Server.DevAddr));
				switch(pstServerMsgUp->Msg.stConfirm2Server.enConfirm2Server)
				{
					case en_Confirm2ServerSuccess:
						json_object_object_add(pragma,"Result",json_object_new_string("Trasmit Success"));
						break;
					case en_Confirm2ServerRadioBusy:
						json_object_object_add(pragma,"Result",json_object_new_string("Radio Busy"));
						break;
					case en_Confirm2ServerOffline:
						json_object_object_add(pragma,"Result",json_object_new_string("Not Active"));
						break;
					case en_Confirm2ServerTooLong:
						json_object_object_add(pragma,"Result",json_object_new_string("Data Too Long"));
						break;
					case en_Confirm2ServerPortNotAllow:
						json_object_object_add(pragma,"Result",json_object_new_string("Port Error"));
						break;
					case en_Confirm2ServerInLastDutyCycle:
						json_object_object_add(pragma,"Result",json_object_new_string("Last Pkg Is Sending"));
						break;
					case en_Confirm2ServerNodeNoAck:
						json_object_object_add(pragma,"Result",json_object_new_string("Node Have No Ack"));
						break;
					case en_Confirm2ServerNodeNotOnRxWindow:
						json_object_object_add(pragma,"Result",json_object_new_string("Rx Windows off"));
						break;
					default:
						json_object_object_add(pragma,"Result",json_object_new_string("Unkown Fault"));
						break;
				}
	            //json_object_object_add(pragma,"Rssi",json_object_new_int(pstServerMsgUp->Msg.stConfirm2Server.rssi));
	            //json_object_object_add(pragma,"Snr",json_object_new_double(pstServerMsgUp->Msg.stConfirm2Server.snr));
			}
			else
			{
				printf("%s ,%d\r\n",__func__,__LINE__);
			}
            memset(data.text,0,MSG_Q_BUFFER_SIZE);
            strcpy(data.text,json_object_to_json_string(pragma));
            json_object_put(pragma);
            if(msgsnd(msgid, (void*)&data, strlen(data.text), 0) == -1)
            {
                printf("msgsnd failed\r\n");
                goto creat_msg_q;
            }
        }
        else
        {
            usleep(100000);
        }
    }
}

