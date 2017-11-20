#ifndef __NODE_DATA_BASE_H__
#define __NODE_DATA_BASE_H__
#include "typedef.h"
#include "aes.h"
#include "cmac.h"
#include "LoRaMacCrypto.h"
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
#include "routin.h"
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#define MAX_NODE    100

typedef struct
{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint8_t DevAddr[4];
    uint16_t DevNonce;
    struct tm* LastComunication;
    uint32_t WakeupTime;
    uint32_t RX1_Window;
    uint32_t RX2_Window;
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
}node_pragma_t;

typedef struct
{
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    uint8_t server_ip[4];
    uint16_t server_port;
}gateway_pragma_t;

typedef struct
{
    uint8_t APPEUI[8];
    uint8_t DevEUI[8];
    uint16_t DevNonce;
}node_join_info_t;

int16_t database_node_join(node_join_info_t* node);
gateway_pragma_t gateway_pragma;
node_pragma_t nodebase_node_pragma[MAX_NODE];

#endif
