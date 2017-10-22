#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include "typedef.h"
#include <errno.h>

#define PORT  8890
#define BUFFER_SIZE 1024
extern bool lora_rx_done;
extern uint8_t radio2tcpbuffer[];

void *tcp_client_routin(void *data)
{
    int txlen = 0;
    int rxlen = 0;
    int sock_cli = 0;
    struct sockaddr_in servaddr;
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];
	int fd = *(int*)data;
    /*if(argc!=2)
    {
        printf("usage: client IP \n");
        exit(0);
    }*/

connecting:
    //定义IPV4的TCP连接的套接字描述符
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.149");
    servaddr.sin_port = htons(PORT);  //服务器端口

    printf("connecting..");
    //连接服务器，成功返回0，错误返回-1
    while (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        //perror("connect");
        //exit(1);
        printf("%c",'.');
        sleep(1);
    }
    printf("\r\nconnect server(IP:%s).\n","192.168.1.149");

    //客户端将控制台输入的信息发送给服务器端，服务器原样返回信息
    while(1)// (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        /*txlen = send(sock_cli, sendbuf, strlen(sendbuf),0); ///发送
        if(txlen < 1)
        {
            printf("transmit error ! reconnect\r\n");
            close(sock_cli);
            goto connecting;
        }
        if(strcmp(sendbuf,"exit\n")==0)
        {
            printf("client exited.\n");
            break;
        }*/

        rxlen = recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        if(rxlen < 0)
        {
        	if((errno == EINTR)
				|| (errno ==EWOULDBLOCK)
				|| (errno ==EAGAIN))
        	{
        	}
			else
			{
	            printf("transmit error ! reconnect\r\n");
	            close(sock_cli);
	            goto connecting;
			}
        }
		else
		{
	        fputs(recvbuf, stdout);
			write(fd,recvbuf,rxlen);
	        memset(sendbuf, 0, sizeof(sendbuf));
	        memset(recvbuf, 0, sizeof(recvbuf));
		}
		printf("lora_rx_done : %d\r\n",lora_rx_done);
		if(lora_rx_done)
		{
			rxlen = send(sock_cli, radio2tcpbuffer, 256, 0);
			if(rxlen < 0)
			{
				printf("transmit error ! reconnect\r\n");
				close(sock_cli);
				goto connecting;
			}
			memset(radio2tcpbuffer,0,256);
			lora_rx_done = 0;
		}
		usleep(10000);
    }

    close(sock_cli);
    return 0;
}



