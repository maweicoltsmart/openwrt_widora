#ifndef __NODE_DATA_BASE_H__
#define __NODE_DATA_BASE_H__
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
//#include "routin.h"
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#define MAX_NODE    100

typedef struct
{
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
	uint8_t radiochannel[3];
	uint8_t radiodatarate[3];
    uint32_t server_ip;
    uint32_t server_port;
	uint32_t local_ip;
    uint32_t local_port;
	uint8_t nettype;
}gateway_pragma_t;

typedef struct
{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
}node_join_info_t;

int16_t database_node_join(node_join_info_t* node);
gateway_pragma_t gateway_pragma;

#endif
