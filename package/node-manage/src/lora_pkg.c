#include <time.h>
#include <modbus/modbus.h>
#include "lora_pkg.h"
#include "typedef.h"
#include "LoRaDevOps.h"
#include "GatewayPragma.h"
#include "Server.h"

st_MjDevInfo stMjDevInfo[MAX_DEV_ID];
extern int fd_cdev;
extern modbus_mapping_t *mb_mapping;

uint16_t crc_16(unsigned char *p,unsigned char n)
{
	unsigned char i,j;
	uint16_t tmp;
	uint16_t  crc_16_calculate = 0xffff;
	for (j = 0 ; j < n ; j++)
	{
		crc_16_calculate ^= *(p++);
		for (i = 0 ; i < 8 ; i++)   {
		if (crc_16_calculate & 0x01)
		{
			crc_16_calculate >>= 1;
			crc_16_calculate ^= 0xa001;
		}
			else crc_16_calculate >>= 1;
		}
	}
	tmp = crc_16_calculate >> 8;
	tmp |= crc_16_calculate << 8;
	return tmp;
}

pthread_t get_tid_by_netaddr(uint32_t DevAddr)
{
	uint16_t loop;
	for(loop = 0;loop < MAX_DEV_ID;loop ++)
	{
		if(stMjDevInfo[loop].DevAddr == DevAddr)
		{
			return loop;
		}
	}
	return -1;
}

int16_t get_devid_by_netaddr(uint32_t DevAddr)
{
	uint16_t loop;
	for(loop = 0;loop < MAX_DEV_ID;loop ++)
	{
		if(stMjDevInfo[loop].DevAddr == DevAddr)
		{
			return loop;
		}
	}
	return -1;
}

void *mjlora_pkg_routin(void *data)
{
	int len;
	uint8_t readbuffer[256 + sizeof(st_ServerMsgUp)];
	pst_ServerMsgUp pstServerMsgUp = (pst_ServerMsgUp)readbuffer;
	st_MjLoraPkgUpV1 stMjLoraPkgUpV1;
	st_MjLoraPkgUpV2 stMjLoraPkgUpV2;
	int16_t devidinpkg;
	struct tm *local;
	time_t tt;
	pthread_t pthreadtokill;
	uint16_t crccode;
	//uint16_t loop = 0;

	tzset();//void tzset(void);设置时间环境变量-时区
	memset(stMjDevInfo,0,sizeof(stMjDevInfo));
	while(1)
	{
		if((len = read(fd_cdev,readbuffer,sizeof(readbuffer))) > 0)
		{
			if(mb_mapping == NULL)
			{
				continue;
			}
			if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramDataReceive)
			{
				//printf("rx one pkg!\r\n");
				if(pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack)
				{
					/*
					pthreadtokill = get_tid_by_netaddr(pstServerMsgUp->Msg.stData2Server.DevAddr);
					//pthread_detach(pthreadtokill);
					pthread_kill(pthreadtokill,0);
					printf("pthread killed!\r\n");
					*/
				}
				pstServerMsgUp->Msg.stData2Server.payload = &readbuffer[sizeof(st_ServerMsgUp)];
				tt=time(NULL);//等价于time(&tt);
				local=localtime(&tt);
				if(pstServerMsgUp->Msg.stData2Server.fPort == UP_DATA_PORT_V1)
				{
					if(pstServerMsgUp->Msg.stData2Server.size != 11)
					{
						printf("length error! length = %d\r\n",pstServerMsgUp->Msg.stData2Server.size);
						continue;
					}
					crccode = crc_16(pstServerMsgUp->Msg.stData2Server.payload,pstServerMsgUp->Msg.stData2Server.size - 2);

					stMjLoraPkgUpV1.devid = (pstServerMsgUp->Msg.stData2Server.payload[0] << 8) | pstServerMsgUp->Msg.stData2Server.payload[1];
					stMjLoraPkgUpV1.Temperature = (pstServerMsgUp->Msg.stData2Server.payload[3] << 8) | pstServerMsgUp->Msg.stData2Server.payload[4];
					stMjLoraPkgUpV1.humidity = (pstServerMsgUp->Msg.stData2Server.payload[5] << 8) | pstServerMsgUp->Msg.stData2Server.payload[6];
					stMjLoraPkgUpV1.Voltage = (pstServerMsgUp->Msg.stData2Server.payload[7] << 8) | pstServerMsgUp->Msg.stData2Server.payload[8];
					stMjLoraPkgUpV1.crc = (pstServerMsgUp->Msg.stData2Server.payload[9] << 8) | pstServerMsgUp->Msg.stData2Server.payload[10];
					//printf("%d,%d,%d,%d\r\n",stMjLoraPkgUpV1.devid,stMjLoraPkgUpV1.Temperature,stMjLoraPkgUpV1.humidity,stMjLoraPkgUpV1.Voltage);
					
					/*stMjLoraPkgUpV1.devid = swapuint16(stMjLoraPkgUpV1.devid);
					stMjLoraPkgUpV1.Temperature = swapuint16(stMjLoraPkgUpV1.Temperature);
					stMjLoraPkgUpV1.humidity = swapuint16(stMjLoraPkgUpV1.humidity);
					stMjLoraPkgUpV1.Voltage = swapuint16(stMjLoraPkgUpV1.Voltage);
					stMjLoraPkgUpV1.crc = swapuint16(stMjLoraPkgUpV1.crc);
					printf("%d,%d,%d,%d\r\n",stMjLoraPkgUpV1.devid,stMjLoraPkgUpV1.Temperature,stMjLoraPkgUpV1.humidity,stMjLoraPkgUpV1.Voltage);
					*/
					//for(loop = 0;loop < 11;loop ++)
					//	printf("")
					if(crccode != stMjLoraPkgUpV1.crc)
					{
						printf("crc error! crccode = 0x%04x crc = 0x%04x\r\n",crccode,stMjLoraPkgUpV1.crc);
						continue;
					}
					devidinpkg = stMjLoraPkgUpV1.devid;
					if(devidinpkg > 399)
					{
						printf("devid error! devid = %d\r\n",devidinpkg);
						continue;
					}
					stMjDevInfo[devidinpkg].fPort = pstServerMsgUp->Msg.stData2Server.fPort;
					stMjDevInfo[devidinpkg].devid = devidinpkg;
					stMjDevInfo[devidinpkg].DevAddr = pstServerMsgUp->Msg.stData2Server.DevAddr;
					stMjDevInfo[devidinpkg].AckRequest = pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest;
					stMjDevInfo[devidinpkg].ClassType = pstServerMsgUp->Msg.stData2Server.ClassType;
					stMjDevInfo[devidinpkg].rssi = pstServerMsgUp->Msg.stData2Server.rssi;
					stMjDevInfo[devidinpkg].snr = pstServerMsgUp->Msg.stData2Server.snr;
					stMjDevInfo[devidinpkg].Ack = pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack;
					mb_mapping->tab_input_registers[1 + devidinpkg * 25 - 1] = local->tm_year + 1900;
					mb_mapping->tab_input_registers[2 + devidinpkg * 25 - 1] = local->tm_mon + 1;
					mb_mapping->tab_input_registers[3 + devidinpkg * 25 - 1] = local->tm_mday;
					mb_mapping->tab_input_registers[4 + devidinpkg * 25 - 1] = local->tm_hour;
					mb_mapping->tab_input_registers[5 + devidinpkg * 25 - 1] = local->tm_min;
					mb_mapping->tab_input_registers[6 + devidinpkg * 25 - 1] = local->tm_sec;
					mb_mapping->tab_input_registers[7 + devidinpkg * 25 - 1] = stMjDevInfo[devidinpkg].rssi;
					mb_mapping->tab_input_registers[8 + devidinpkg * 25 - 1] = stMjDevInfo[devidinpkg].snr;
					mb_mapping->tab_input_registers[9 + devidinpkg * 25 - 1] = stMjLoraPkgUpV1.Temperature;
					mb_mapping->tab_input_registers[10 + devidinpkg * 25 - 1] = stMjLoraPkgUpV1.Temperature;
					mb_mapping->tab_input_registers[11 + devidinpkg * 25 - 1] = stMjLoraPkgUpV1.humidity;
					mb_mapping->tab_input_registers[12 + devidinpkg * 25 - 1] = stMjLoraPkgUpV1.humidity;
					mb_mapping->tab_input_registers[25 + devidinpkg * 25 - 1] = stMjLoraPkgUpV1.Voltage;
					//pthread_create(&stMjDevInfo[devidinpkg].datadownprocess, NULL, mjlora_data_down_routin, &devidinpkg);
					mjlora_data_down_routin(&devidinpkg);
				}
				else if(pstServerMsgUp->Msg.stData2Server.fPort == UP_DATA_PORT_V2)
				{
					if(pstServerMsgUp->Msg.stData2Server.size != 39)
					{
						printf("length error! length = %d\r\n",pstServerMsgUp->Msg.stData2Server.size);
						continue;
					}
					crccode = crc_16(pstServerMsgUp->Msg.stData2Server.payload,pstServerMsgUp->Msg.stData2Server.size - 2);
					stMjLoraPkgUpV2.devid = (pstServerMsgUp->Msg.stData2Server.payload[0] << 8) | pstServerMsgUp->Msg.stData2Server.payload[1];
					stMjLoraPkgUpV2.Temperature1 = (pstServerMsgUp->Msg.stData2Server.payload[3] << 8) | pstServerMsgUp->Msg.stData2Server.payload[4];
					stMjLoraPkgUpV2.Temperature2 = (pstServerMsgUp->Msg.stData2Server.payload[5] << 8) | pstServerMsgUp->Msg.stData2Server.payload[6];
					stMjLoraPkgUpV2.humidity1 = (pstServerMsgUp->Msg.stData2Server.payload[7] << 8) | pstServerMsgUp->Msg.stData2Server.payload[8];
					stMjLoraPkgUpV2.humidity2 = (pstServerMsgUp->Msg.stData2Server.payload[9] << 8) | pstServerMsgUp->Msg.stData2Server.payload[10];
					stMjLoraPkgUpV2.pressure1 = (pstServerMsgUp->Msg.stData2Server.payload[11] << 8) | pstServerMsgUp->Msg.stData2Server.payload[12];
					stMjLoraPkgUpV2.pressure2 = (pstServerMsgUp->Msg.stData2Server.payload[13] << 8) | pstServerMsgUp->Msg.stData2Server.payload[14];
					stMjLoraPkgUpV2.pressure3 = (pstServerMsgUp->Msg.stData2Server.payload[15] << 8) | pstServerMsgUp->Msg.stData2Server.payload[16];
					stMjLoraPkgUpV2.pressure4 = (pstServerMsgUp->Msg.stData2Server.payload[17] << 8) | pstServerMsgUp->Msg.stData2Server.payload[18];
					stMjLoraPkgUpV2.pressure5 = (pstServerMsgUp->Msg.stData2Server.payload[19] << 8) | pstServerMsgUp->Msg.stData2Server.payload[20];
					stMjLoraPkgUpV2.pressure6 = (pstServerMsgUp->Msg.stData2Server.payload[21] << 8) | pstServerMsgUp->Msg.stData2Server.payload[22];
					stMjLoraPkgUpV2.pressure7 = (pstServerMsgUp->Msg.stData2Server.payload[23] << 8) | pstServerMsgUp->Msg.stData2Server.payload[24];
					stMjLoraPkgUpV2.pressure8 = (pstServerMsgUp->Msg.stData2Server.payload[25] << 8) | pstServerMsgUp->Msg.stData2Server.payload[26];
					stMjLoraPkgUpV2.pressure9 = (pstServerMsgUp->Msg.stData2Server.payload[27] << 8) | pstServerMsgUp->Msg.stData2Server.payload[28];
					stMjLoraPkgUpV2.pressure10 = (pstServerMsgUp->Msg.stData2Server.payload[29] << 8) | pstServerMsgUp->Msg.stData2Server.payload[30];
					stMjLoraPkgUpV2.reserve1 = (pstServerMsgUp->Msg.stData2Server.payload[31] << 8) | pstServerMsgUp->Msg.stData2Server.payload[32];
					stMjLoraPkgUpV2.reserve2 = (pstServerMsgUp->Msg.stData2Server.payload[33] << 8) | pstServerMsgUp->Msg.stData2Server.payload[34];
					stMjLoraPkgUpV2.Voltage = (pstServerMsgUp->Msg.stData2Server.payload[35] << 8) | pstServerMsgUp->Msg.stData2Server.payload[36];
					stMjLoraPkgUpV2.crc = (pstServerMsgUp->Msg.stData2Server.payload[37] << 8) | pstServerMsgUp->Msg.stData2Server.payload[38];

					if(crccode != stMjLoraPkgUpV2.crc)
					{
						printf("crc error! crccode = 0x%04x crc = 0x%04x\r\n",crccode,stMjLoraPkgUpV2.crc);
						continue;
					}
					devidinpkg = stMjLoraPkgUpV2.devid;
					if(devidinpkg > 399)
					{
						printf("devid error! devid = %d\r\n",devidinpkg);
						continue;
					}
					stMjDevInfo[devidinpkg].fPort = pstServerMsgUp->Msg.stData2Server.fPort;
					stMjDevInfo[devidinpkg].devid = devidinpkg;
					stMjDevInfo[devidinpkg].DevAddr = pstServerMsgUp->Msg.stData2Server.DevAddr;
					stMjDevInfo[devidinpkg].AckRequest = pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest;
					stMjDevInfo[devidinpkg].ClassType = pstServerMsgUp->Msg.stData2Server.ClassType;
					stMjDevInfo[devidinpkg].rssi = pstServerMsgUp->Msg.stData2Server.rssi;
					stMjDevInfo[devidinpkg].snr = pstServerMsgUp->Msg.stData2Server.snr;
					stMjDevInfo[devidinpkg].Ack = pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack;
					mb_mapping->tab_input_registers[1 + devidinpkg * 25 - 1] = local->tm_year + 1900;
					mb_mapping->tab_input_registers[2 + devidinpkg * 25 - 1] = local->tm_mon + 1;
					mb_mapping->tab_input_registers[3 + devidinpkg * 25 - 1] = local->tm_mday;
					mb_mapping->tab_input_registers[4 + devidinpkg * 25 - 1] = local->tm_hour;
					mb_mapping->tab_input_registers[5 + devidinpkg * 25 - 1] = local->tm_min;
					mb_mapping->tab_input_registers[6 + devidinpkg * 25 - 1] = local->tm_sec;
					mb_mapping->tab_input_registers[7 + devidinpkg * 25 - 1] = stMjDevInfo[devidinpkg].rssi;
					mb_mapping->tab_input_registers[8 + devidinpkg * 25 - 1] = stMjDevInfo[devidinpkg].snr;
					mb_mapping->tab_input_registers[9 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.Temperature1;
					mb_mapping->tab_input_registers[10 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.Temperature2;
					mb_mapping->tab_input_registers[11 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.humidity1;
					mb_mapping->tab_input_registers[12 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.humidity2;
					mb_mapping->tab_input_registers[13 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure1;
					mb_mapping->tab_input_registers[14 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure2;
					mb_mapping->tab_input_registers[15 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure3;
					mb_mapping->tab_input_registers[16 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure4;
					mb_mapping->tab_input_registers[17 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure5;
					mb_mapping->tab_input_registers[18 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure6;
					mb_mapping->tab_input_registers[19 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure7;
					mb_mapping->tab_input_registers[20 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure8;
					mb_mapping->tab_input_registers[21 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure9;
					mb_mapping->tab_input_registers[22 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.pressure10;
					mb_mapping->tab_input_registers[23 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.reserve1;
					mb_mapping->tab_input_registers[24 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.reserve2;
					mb_mapping->tab_input_registers[25 + devidinpkg * 25 - 1] = stMjLoraPkgUpV2.Voltage;
					//pthread_create(&stMjDevInfo[devidinpkg].datadownprocess, NULL, mjlora_data_down_routin, &devidinpkg);
					mjlora_data_down_routin(&devidinpkg);
				}
				else
				{
					devidinpkg = get_devid_by_netaddr(pstServerMsgUp->Msg.stData2Server.DevAddr);
					if((devidinpkg > 399) || (devidinpkg < 0))
					{
						continue;
					}
					stMjDevInfo[devidinpkg].AckRequest = pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest;
					mb_mapping->tab_input_registers[1 + devidinpkg * 25 - 1] = local->tm_year + 1900;
					mb_mapping->tab_input_registers[2 + devidinpkg * 25 - 1] = local->tm_mon + 1;
					mb_mapping->tab_input_registers[3 + devidinpkg * 25 - 1] = local->tm_mday;
					mb_mapping->tab_input_registers[4 + devidinpkg * 25 - 1] = local->tm_hour;
					mb_mapping->tab_input_registers[5 + devidinpkg * 25 - 1] = local->tm_min;
					mb_mapping->tab_input_registers[6 + devidinpkg * 25 - 1] = local->tm_sec;
					mb_mapping->tab_input_registers[7 + devidinpkg * 25 - 1] = pstServerMsgUp->Msg.stData2Server.rssi;
					mb_mapping->tab_input_registers[8 + devidinpkg * 25 - 1] = pstServerMsgUp->Msg.stData2Server.snr;
					mjlora_data_down_routin(&devidinpkg);
				}
			}
		}
		else
		{
			usleep(100000);
		}
	}
}

void *mjlora_data_down_routin(void *data)
{
	uint16_t devid = *(uint16_t*)data;
	uint8_t writebuf[256 + sizeof(st_ServerMsgDown)];
	uint32_t sendlen;
	pst_ServerMsgDown pstServerMsgDown;
	uint16_t crccode;

	//usleep(900000);
	pstServerMsgDown = (pst_ServerMsgDown)writebuf;
	pstServerMsgDown->enMsgDownFramType = en_MsgDownFramDataSend;
	pstServerMsgDown->Msg.stData2Node.payload = writebuf + sizeof(st_ServerMsgDown);
	pstServerMsgDown->Msg.stData2Node.DevAddr = stMjDevInfo[devid].DevAddr;
	pstServerMsgDown->Msg.stData2Node.fPort = stMjDevInfo[devid].fPort;
	pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest = false;
	if(stMjDevInfo[devid].AckRequest)
	{
		pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = true;
	}
	else
	{
		pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = false;
	}
	if(stMjDevInfo[devid].fPort == UP_DATA_PORT_V1)
	{
		pstServerMsgDown->Msg.stData2Node.size = 5;
		pstServerMsgDown->Msg.stData2Node.payload[0] = 0x06;
		pstServerMsgDown->Msg.stData2Node.payload[1] = (uint8_t)mb_mapping->tab_registers[1 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[2] = (uint8_t)(mb_mapping->tab_registers[1 + devid * 25 - 1] & 0xff);
		crccode = crc_16(pstServerMsgDown->Msg.stData2Node.payload,pstServerMsgDown->Msg.stData2Node.size - 2);
		pstServerMsgDown->Msg.stData2Node.payload[3] = crccode >> 8; // crc
		pstServerMsgDown->Msg.stData2Node.payload[4] = crccode; // crc
		//printf("crccode = 0x%04x,line = %d\r\n",crccode,__LINE__);
	}
	else if(stMjDevInfo[devid].fPort == UP_DATA_PORT_V2)
	{
		pstServerMsgDown->Msg.stData2Node.size = 11;
		pstServerMsgDown->Msg.stData2Node.payload[0] = 0x10;
		pstServerMsgDown->Msg.stData2Node.payload[1] = (uint8_t)mb_mapping->tab_registers[1 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[2] = (uint8_t)(mb_mapping->tab_registers[1 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[3] = (uint8_t)mb_mapping->tab_registers[2 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[4] = (uint8_t)(mb_mapping->tab_registers[2 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[5] = (uint8_t)mb_mapping->tab_registers[3 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[6] = (uint8_t)(mb_mapping->tab_registers[3 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[7] = (uint8_t)mb_mapping->tab_registers[4 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[8] = (uint8_t)(mb_mapping->tab_registers[4 + devid * 25 - 1] & 0xff);
		crccode = crc_16(pstServerMsgDown->Msg.stData2Node.payload,pstServerMsgDown->Msg.stData2Node.size - 2);
		pstServerMsgDown->Msg.stData2Node.payload[9] = crccode >> 8; // crc
		pstServerMsgDown->Msg.stData2Node.payload[10] = crccode; // crc
		//printf("crccode = 0x%04x,line = %d\r\n",crccode,__LINE__);
	}
	else
	{
		pstServerMsgDown->Msg.stData2Node.size = 0;
	}
	sendlen = pstServerMsgDown->Msg.stData2Node.size + sizeof(st_ServerMsgDown);
	
	write(fd_cdev,writebuf,sendlen);
#if 0
	usleep(1000000); // retry in 2nd rx window
	pstServerMsgDown = (pst_ServerMsgDown)writebuf;
	pstServerMsgDown->enMsgDownFramType = en_MsgDownFramDataSend;
	pstServerMsgDown->Msg.stData2Node.payload = writebuf + sizeof(st_ServerMsgDown);
	pstServerMsgDown->Msg.stData2Node.DevAddr = stMjDevInfo[devid].DevAddr;
	pstServerMsgDown->Msg.stData2Node.fPort = stMjDevInfo[devid].fPort;
	pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest = true;
	if(stMjDevInfo[devid].AckRequest)
	{
		pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = true;
	}
	else
	{
		pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = false;
	}
	if(stMjDevInfo[devid].fPort == UP_DATA_PORT_V1)
	{
		pstServerMsgDown->Msg.stData2Node.size = 5;
		pstServerMsgDown->Msg.stData2Node.payload[0] = 0x06;
		pstServerMsgDown->Msg.stData2Node.payload[1] = (uint8_t)mb_mapping->tab_registers[1 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[2] = (uint8_t)(mb_mapping->tab_registers[1 + devid * 25 - 1] & 0xff);
		crccode = crc_16(pstServerMsgDown->Msg.stData2Node.payload,pstServerMsgDown->Msg.stData2Node.size - 2);
		pstServerMsgDown->Msg.stData2Node.payload[3] = crccode >> 8; // crc
		pstServerMsgDown->Msg.stData2Node.payload[4] = crccode; // crc
		printf("crccode = 0x%04x,line = %d\r\n",crccode,__LINE__);
	}
	else if(stMjDevInfo[devid].fPort == UP_DATA_PORT_V2)
	{
		pstServerMsgDown->Msg.stData2Node.size = 11;
		pstServerMsgDown->Msg.stData2Node.payload[0] = 0x10;
		pstServerMsgDown->Msg.stData2Node.payload[1] = (uint8_t)mb_mapping->tab_registers[1 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[2] = (uint8_t)(mb_mapping->tab_registers[1 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[3] = (uint8_t)mb_mapping->tab_registers[2 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[4] = (uint8_t)(mb_mapping->tab_registers[2 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[5] = (uint8_t)mb_mapping->tab_registers[3 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[6] = (uint8_t)(mb_mapping->tab_registers[3 + devid * 25 - 1] & 0xff);
		pstServerMsgDown->Msg.stData2Node.payload[7] = (uint8_t)mb_mapping->tab_registers[4 + devid * 25 - 1] >> 8;
		pstServerMsgDown->Msg.stData2Node.payload[8] = (uint8_t)(mb_mapping->tab_registers[4 + devid * 25 - 1] & 0xff);
		crccode = crc_16(pstServerMsgDown->Msg.stData2Node.payload,pstServerMsgDown->Msg.stData2Node.size - 2);
		pstServerMsgDown->Msg.stData2Node.payload[9] = crccode >> 8; // crc
		pstServerMsgDown->Msg.stData2Node.payload[10] = crccode; // crc
		printf("crccode = 0x%04x,line = %d\r\n",crccode,__LINE__);
	}
	else
	{
		pstServerMsgDown->Msg.stData2Node.size = 0;
	}
	sendlen = pstServerMsgDown->Msg.stData2Node.size + sizeof(st_ServerMsgDown);
	
	write(fd_cdev,writebuf,sendlen);
	pthread_detach(pthread_self());
	//printf("pthread quite!\r\n");
#endif
}