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

#define PORT  8890
#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
    int txlen = 0;
    int rxlen = 0;
    int sock_cli = 0;
    struct sockaddr_in servaddr;
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    if(argc!=2)
    {
        printf("usage: client IP \n");
        exit(0);
    }

connecting:
    //定义IPV4的TCP连接的套接字描述符
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
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
    printf("\r\nconnect server(IP:%s).\n",argv[1]);

    //客户端将控制台输入的信息发送给服务器端，服务器原样返回信息
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        txlen = send(sock_cli, sendbuf, strlen(sendbuf),MSG_DONTWAIT); ///发送
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
        }
        printf("client receive:\n");
        rxlen = recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        if(rxlen < 1)
        {
            printf("transmit error ! reconnect\r\n");
            close(sock_cli);
            goto connecting;
        }
        fputs(recvbuf, stdout);

        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    close(sock_cli);
    return 0;
}



