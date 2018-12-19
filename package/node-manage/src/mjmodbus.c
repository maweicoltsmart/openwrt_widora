#include <unistd.h>
#include <modbus/modbus.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <string.h>
#include <json-c/json.h>
#include <errno.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "mjmodbus.h"
#include "typedef.h"
#include "LoRaDevOps.h"
#include "GatewayPragma.h"
#include "Server.h"

modbus_mapping_t *mb_mapping = NULL;
unsigned char streth0ipaddr[16 + 1] = {0};

int get_eth0_ip_address(void)
{
    struct ifaddrs* ifaddr = NULL;
    int ret = getifaddrs(&ifaddr);

    if (ret) {
        printf("getifaddrs failed, errno:%d\n", errno);
        return -1;
    }
      
    struct ifaddrs* ifp = ifaddr;
    char ip[16];
    char netmask[16];

    for ( ; ifp != NULL; ifp = ifp->ifa_next) {
        if (ifp->ifa_addr && ifp->ifa_addr->sa_family == AF_INET) {
            strncpy(ip, inet_ntoa(((struct sockaddr_in*)ifp->ifa_addr)->sin_addr), 16);
            strncpy(netmask, inet_ntoa(((struct sockaddr_in*)ifp->ifa_netmask)->sin_addr), 16);
            if(strcmp("eth0",ifp->ifa_name) == 0)
            {
                memset(streth0ipaddr,0,sizeof(streth0ipaddr));
                strcpy(streth0ipaddr,ip);
                return 0;
            }
            //printf("dev:%s, ip:%s, netmask:%s\n", ifp->ifa_name, ip, netmask);
        }
    }

    freeifaddrs(ifaddr);
    return -1;
}

void *mjmodbus_server_routin(void *data)
{
	int socket;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping_server = NULL;
    int loop;

createctx:
	while(get_eth0_ip_address() < 0)
	{
		sleep(1);
	}
	printf("creat modbus tcp with ip: %s\r\n",streth0ipaddr);
    ctx = modbus_new_tcp(streth0ipaddr, 502);
    /* modbus_set_debug(ctx, TRUE); */

    mb_mapping_server = modbus_mapping_new(10000, 10000, 10000, 10000);
    if (mb_mapping_server == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        //return -1;
        sleep(1);
        goto createctx;
    }
    for(loop = 0;loop < 10000;loop ++)
    {
    	mb_mapping_server->tab_input_registers[loop] = 30000 + loop;
    	mb_mapping_server->tab_registers[loop] = loop;
    }
    modbus_set_debug(ctx, TRUE);
	printf("listen!\r\n");
    socket = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &socket);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;
        mb_mapping = mb_mapping_server;
        rc = modbus_receive(ctx, query);
        if (rc != -1) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping_server);
        } else {
            /* Connection closed by the client or error */
            break;
        }
    }
    //sleep(1);
    printf("Quit the loop: %s\n", modbus_strerror(errno));
    mb_mapping = NULL;
    modbus_mapping_free(mb_mapping_server);
    
    modbus_close(ctx);
    modbus_free(ctx);
    close(socket);
    goto createctx;
    //system("/sbin/reboot");
    return 0;
}

void *mjmodbus_slave_routin(void *data)
{
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping_server = NULL;
    int loop;

createctx:
    ctx = modbus_new_rtu("/dev/ttyS2", 115200, 'N', 8, 1);
    /* modbus_set_debug(ctx, TRUE); */
    modbus_set_debug(ctx, TRUE);
    modbus_set_slave(ctx, SERVER_ID);
    mb_mapping_server = modbus_mapping_new(10000, 10000, 10000, 10000);
    if (mb_mapping_server == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        //return -1;
        sleep(1);
        goto createctx;
    }
    for(loop = 0;loop < 10000;loop ++)
    {
    	mb_mapping_server->tab_input_registers[loop] = 20000 + loop;
    	mb_mapping_server->tab_registers[loop] = loop;
    }
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_mapping_free(mb_mapping_server);
        modbus_free(ctx);
        printf("connection failed\r\n");
        goto createctx;
    }
    printf("receive & reply\r\n");
    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;
        mb_mapping = mb_mapping_server;
        rc = modbus_receive(ctx, query);
        if (rc != -1) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping_server);
        } else {
            /* Connection closed by the client or error */
            break;
        }
    }
    //sleep(1);
    printf("Quit the loop: %s\n", modbus_strerror(errno));
    mb_mapping = NULL;
    modbus_mapping_free(mb_mapping_server);
    
    modbus_close(ctx);
    modbus_free(ctx);
    goto createctx;
    //system("/sbin/reboot");
    return 0;
}