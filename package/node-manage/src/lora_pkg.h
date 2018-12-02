#ifndef __MJ_LORA_PKG_H__
#define __MJ_LORA_PKG_H__
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <signal.h>  
#include <string.h>  
#include <pthread.h>

#define UP_DATA_PORT_V1			100
#define UP_DATA_PORT_V2			101
#define DOWN_DATA_PORT_V1		100
#define DOWN_DATA_PORT_V2		101
#define MAX_DEV_ID				500

typedef struct{
	uint16_t devid;
	uint32_t DevAddr;
	bool AckRequest;
	bool Ack;
	uint8_t ClassType;
	uint8_t fPort;
	int16_t rssi;
    int8_t snr;
	uint16_t year;
	uint16_t month;
	uint16_t date;
	uint16_t hour;
	uint16_t minite;
	uint16_t second;
	pthread_t datadownprocess;
	pthread_attr_t attr;
}st_MjDevInfo,*pst_MjDevInfo;

typedef struct{
	uint16_t devid;
	uint8_t cmd;
	uint16_t Temperature;
	uint16_t humidity;
	uint16_t crc;
}st_MjLoraPkgUpV1,*pst_MjLoraPkgUpV1;

typedef struct{
	uint16_t devid;
	uint8_t cmd;
	uint16_t Temperature[2];
	uint16_t humidity[2];
	uint16_t pressure[10];
	uint16_t reserve[2];
	uint16_t Voltage;
	uint16_t crc;
}st_MjLoraPkgUpV2,*pst_MjLoraPkgUpV2;

typedef struct{
	uint8_t cmd; // fix value 0x06
	uint16_t cycle;
	uint16_t crc;
}st_MjLoraPkgDownV1,*pst_MjLoraPkgDownV1;

typedef struct{
	uint8_t cmd; // fix value 0x10
	uint16_t cycle;
	uint16_t valve[3];
	uint16_t crc;
}st_MjLoraPkgDownV2,*pst_MjLoraPkgDownV2;

extern st_MjDevInfo stMjDevInfo[];

void *mjlora_pkg_routin(void *data);
void *mjlora_data_down_routin(void *data);

#endif