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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>  
#include <sys/ipc.h>  
#include <sys/stat.h>  
#include <sys/msg.h>  
#include "typedef.h"
#include <errno.h>

#define PORT  8890
char sendbuf[BUFFER_SIZE];
char recvbuf[BUFFER_SIZE];

void *client_send(void *arg)
{
    int sockfd = *(int*)arg;
    unsigned char buffer[1024];
    int len;
    //strcpy(buffer,"hello\r\n");
    //len = strlen(buffer);
	struct msg_st data;  
	long int msgtype = 0; //注意1  
	int msgid = -1; 
	memset(data.text,0,BUFFER_SIZE);
	//建立消息队列  
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);	
	if(msgid == -1)  
	{  
		fprintf(stderr, "msgget failed with error: %d\n", errno);  
		exit(EXIT_FAILURE);  
	}  
	//从队列中获取消息，直到遇到end消息为止  
	while(1)	
	{  
		if(msgrcv(msgid, (void*)&data, BUFFER_SIZE, msgtype, 0) == -1)  
		{  
			fprintf(stderr, "msgrcv failed with errno: %d\n", errno);  
			exit(EXIT_FAILURE);
		}  
		printf("You wrote: %s\n",data.text);  
		len = send(sockfd, data.text, strlen(data.text), 0);
        if(len < 1)
        {
            break;
        }
		memset(data.text,0,BUFFER_SIZE);
	}  
	//删除消息队列  
	if(msgctl(msgid, IPC_RMID, 0) == -1)  
	{  
		fprintf(stderr, "msgctl(IPC_RMID) failed\n");  
		exit(EXIT_FAILURE);
	}  
}

void *tcp_client_routin(void *data)
{
    int len;
    int sock_cli;
    struct sockaddr_in servaddr;
    int err;
    pthread_t main_tid;
    int fd = *(int*)data;
connect:
    //定义IPV4的TCP连接的套接字描述符
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.149");
    servaddr.sin_port = htons(PORT);  //服务器端口
 
    //连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
	close(sock_cli);
	sleep(1);
        goto connect;//exit(1);
    }
    printf("connect server(IP:%s).\r\n","192.168.1.149");  
    err = pthread_create(&main_tid, NULL, client_send, &sock_cli); //创建线程
    strcpy(sendbuf,"hello\r\n");
    //客户端将控制台输入的信息发送给服务器端，服务器原样返回信息
    while(1)// (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        /*len = send(sock_cli, sendbuf, strlen(sendbuf),0); ///发送
	if(len < 1)
	{
	    close(sock_cli);
            goto connect;
	}
        if(strcmp(sendbuf,"exit\n")==0)
        {
	    printf("client exited.\n");
	    close(sock_cli);
            goto connect;
        }
        printf("client receive:\n");*/
        len = recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        if(len < 1)
        {
            close(sock_cli);
            goto connect;
        }
        fputs(recvbuf, stdout);
		write(fd,recvbuf,len);
        //memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
	//sleep(1);
    }

    close(sock_cli);
    return 0;
}
