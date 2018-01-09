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
#include <unistd.h>   //sleep
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include "typedef.h"
//#include "nodedatabase.h"
#include "nodedatabase.h"
#include "LoRaDevOps.h"
#include "RegionCN470.h"
#include "LoRaMac.h"
#include "sx1276.h"
#include "Region.h"
#include "GatewayPragma.h"

int fd_proc_cfg_rx,fd_proc_cfg_tx;
int fd_cdev = 0;


static uint8_t *LoRaMacAppKey;
LoRaMacParams_t LoRaMacParams;
LoRaMacParams_t LoRaMacParamsDefaults;

#define LORADEV_IOC_MAGIC  'r'

#define LORADEV_IOCPRINT   _IO(LORADEV_IOC_MAGIC, 0)  //没参数
#define LORADEV_IOCGETDATA _IOR(LORADEV_IOC_MAGIC, 1, int)  //读
#define LORADEV_IOCSETDATA _IOW(LORADEV_IOC_MAGIC, 2, int)  //写
#define LORADEV_RADIO_INIT   _IOW(LORADEV_IOC_MAGIC, 3, int)  //没参数
#define LORADEV_RADIO_STATE _IOW(LORADEV_IOC_MAGIC, 4, int)  //读
#define LORADEV_RADIO_CHANNEL _IOW(LORADEV_IOC_MAGIC, 5, int)  //写
#define LORADEV_RADIO_SET_PUBLIC _IOW(LORADEV_IOC_MAGIC, 6, int)  //写
#define LORADEV_RADIO_SET_MODEM _IOW(LORADEV_IOC_MAGIC, 7, int)  //写
#define LORADEV_RADIO_READ_REG _IOWR(LORADEV_IOC_MAGIC, 8, int)  //写
#define LORADEV_RADIO_SET_TXCFG _IOW(LORADEV_IOC_MAGIC, 9, int)
#define LORADEV_RADIO_SET_RXCFG _IOW(LORADEV_IOC_MAGIC, 10, int)
#define LORADEV_RADIO_SET_RX _IOW(LORADEV_IOC_MAGIC, 11, int)
#define LORADEV_RADIO_SET_TX _IOW(LORADEV_IOC_MAGIC, 12, int)
#define LORADEV_RADIO_SET_SLEEP _IOW(LORADEV_IOC_MAGIC, 13, int)
#define LORADEV_RADIO_SET_STDBY _IOW(LORADEV_IOC_MAGIC, 14, int)

#define LORADEV_IOC_MAXNR 15

void LoRaMacInit(void)
{
    int chip = 0;
    int i;
    st_RadioCfg stRadioCfg;
    //const uint8_t symbpreamble[12] = {0,1,2,3,4,5,6,24,14,9,};
    GetGatewayPragma();

    fd_proc_cfg_rx = open("/proc/lora_procfs/lora_cfg_rx",O_RDWR);
    if (fd_proc_cfg_rx < 0)
    {
        perror("lora_proc_cfg_rx");
        printf("open lora_proc_cfg_rx error\r\n");
        return;
    }
    fd_proc_cfg_tx = open("/proc/lora_procfs/lora_cfg_tx",O_RDWR);
    if (fd_proc_cfg_tx < 0)
    {
        perror("lora_proc_cfg_tx");
        printf("open lora_proc_cfg_tx error\r\n");
        return;
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

    sleep(1);
    fd_cdev = open("/dev/lora_radio",O_RDWR | O_NONBLOCK);
    if (fd_cdev < 0)
    {
        printf("open lora_radio error\r\n");
        return;
    }
/*
    int ioarg = false;
    ioarg = ioarg | (chip << 31);
    ioctl(fd_cdev, LORADEV_RADIO_SET_PUBLIC, &ioarg);

    //uint32_t channel,datarate;
    uint32_t datarate;
    for(i = 0;i < 2;i ++)
    {
        chip = i;
        ioarg = 0;
        ioarg = ioarg | (chip << 8);    // channel
        datarate = DataratesCN470[5];
        ioarg = ioarg | (datarate & 0x000000ff);    // datarate
        ioarg = ioarg | (chip << 31);
        ioctl(fd_cdev, LORADEV_RADIO_CHANNEL, &ioarg);

        ioarg = 0;
        ioarg = ioarg | (chip << 31);
        ioctl(fd_cdev, LORADEV_RADIO_SET_RX, &ioarg);
    }
    */
}

uint8_t radiorxbuffer[300];
uint8_t radiotxbuffer[2][300];

void *Radio_routin(void *param){
    LoRaMacPrimitives_t LoRaMacPrimitives;
    LoRaMacCallback_t LoRaMacCallbacks;
    MibRequestConfirm_t mibReq;
    int msgid = -1;
    //struct msg_st data;
    struct msg_st data;
    long int msgtype = 0; //注意1

    int len;
    //int fd = *(int *)data;
    int chip = 0;
    int index;
    //pst_lora_rx_data_type p1;
    //pst_lora_tx_data_type p2,p3;
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t fCtrl;
    uint32_t mic = 0;
    //node_join_info_t node_join_info;
    uint8_t *pkg,*pkg1,*pkg2;
    //printf("%s,%d\r\n",__func__,__LINE__);

    LoRaMacInit();

    creat_msg_q:
    //建立消息队列
    while((msgid = msgget((key_t)1234, 0666 | IPC_CREAT) == -1))
    {
        printf("msgget failed with error: %d\r\n", errno);
        sleep(1);
    }
    while(1)
    {
        //sleep(1);
        //printf("fd_cdev = %d\r\n", fd_cdev);
        if((len = read(fd_cdev,radiorxbuffer,300)) > 0)
        {
            data.msg_type = 1;    //注意2
            memcpy(data.text, radiorxbuffer,len);
            //向队列发送数据
            //printf("send msg\r\n");
            if(msgsnd(msgid, (void*)&data, len, 0) == -1)
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

