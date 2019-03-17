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
    for(loop = 0;loop <= 399;loop ++)
    {
    	mb_mapping_server->tab_input_registers[25 * loop + 0] = 1900;
        mb_mapping_server->tab_input_registers[25 * loop + 1] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 2] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 3] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 4] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 5] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 6] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 7] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 8] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 9] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 10] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 11] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 12] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 13] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 14] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 15] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 16] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 17] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 18] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 19] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 20] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 21] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 22] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 23] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 24] = 0;
    	mb_mapping_server->tab_registers[25 * loop + 0] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 1] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 2] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 3] = 30000;
    }
    modbus_set_debug(ctx, TRUE);
    modbus_set_slave(ctx, gateway_pragma.slaveid);
	printf("listen!\r\n");
    socket = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &socket);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;
        mb_mapping = mb_mapping_server;
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping_server);
        }  else if (rc == -1) {
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

void *mjmodbus_slave485_routin(void *data)
{
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping_server = NULL;
    int loop;

createctx:
    ctx = modbus_new_rtu("/dev/ttyUSB0", gateway_pragma.baud, gateway_pragma.parity, 8, 1);
    /* modbus_set_debug(ctx, TRUE); */
    modbus_set_debug(ctx, false);
    modbus_set_slave(ctx, gateway_pragma.slaveid);
    printf("SlaveID is %d\r\n",gateway_pragma.slaveid);
    mb_mapping_server = modbus_mapping_new(10000, 10000, 10000, 10000);
    if (mb_mapping_server == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        //return -1;
        sleep(1);
        goto createctx;
    }
    for(loop = 0;loop <= 399;loop ++)
    {
        mb_mapping_server->tab_input_registers[25 * loop + 0] = 1900;
        mb_mapping_server->tab_input_registers[25 * loop + 1] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 2] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 3] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 4] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 5] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 6] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 7] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 8] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 9] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 10] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 11] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 12] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 13] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 14] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 15] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 16] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 17] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 18] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 19] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 20] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 21] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 22] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 23] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 24] = 0;
        mb_mapping_server->tab_registers[25 * loop + 0] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 1] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 2] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 3] = 30000;
    }
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_mapping_free(mb_mapping_server);
        modbus_free(ctx);
        printf("connection failed\r\n");
        sleep(2);
        goto createctx;
    }
    printf("receive & reply\r\n");
    for (;;) {
        uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
        int rc;
        mb_mapping = mb_mapping_server;
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping_server);
        }  else if (rc == -1) {
            /* Connection closed by the client or error */
            // break;
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

void *mjmodbus_slave232_routin(void *data)
{
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping_server = NULL;
    int loop;

createctx:
    ctx = modbus_new_rtu("/dev/ttyS2", gateway_pragma.baud, gateway_pragma.parity, 8, 1);
    /* modbus_set_debug(ctx, TRUE); */
    modbus_set_debug(ctx, false);
    modbus_set_slave(ctx, gateway_pragma.slaveid);
    printf("SlaveID is %d\r\n",gateway_pragma.slaveid);
    mb_mapping_server = modbus_mapping_new(10000, 10000, 10000, 10000);
    if (mb_mapping_server == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        //return -1;
        sleep(1);
        goto createctx;
    }
    for(loop = 0;loop <= 399;loop ++)
    {
        mb_mapping_server->tab_input_registers[25 * loop + 0] = 1900;
        mb_mapping_server->tab_input_registers[25 * loop + 1] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 2] = 1;
        mb_mapping_server->tab_input_registers[25 * loop + 3] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 4] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 5] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 6] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 7] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 8] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 9] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 10] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 11] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 12] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 13] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 14] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 15] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 16] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 17] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 18] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 19] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 20] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 21] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 22] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 23] = 0;
        mb_mapping_server->tab_input_registers[25 * loop + 24] = 0;
        mb_mapping_server->tab_registers[25 * loop + 0] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 1] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 2] = 30000;
        mb_mapping_server->tab_registers[25 * loop + 3] = 30000;
    }
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_mapping_free(mb_mapping_server);
        modbus_free(ctx);
        printf("connection failed\r\n");
        sleep(2);
        goto createctx;
    }
    printf("receive & reply\r\n");
    for (;;) {
        uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
        int rc;
        mb_mapping = mb_mapping_server;
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping_server);
        } else if (rc == -1){
            /* Connection closed by the client or error */
            // break;
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