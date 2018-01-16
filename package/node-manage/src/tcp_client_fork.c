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
#include <json-c/json.h>

#include "typedef.h"
#include "LoRaDevOps.h"

#include <errno.h>

#define PORT  8890
char sendbuf[MSG_Q_BUFFER_SIZE];
char recvbuf[MSG_Q_BUFFER_SIZE];
extern int fd_cdev;

void *client_send(void *arg)
{
    int sockfd = *(int*)arg;
    unsigned char buffer[MSG_Q_BUFFER_SIZE];
    int len;
    //strcpy(buffer,"hello\r\n");
    //len = strlen(buffer);
    struct msg_st data;
    long int msgtype = 0; //注意1
    int msgid = -1;
    memset(data.text,0,MSG_Q_BUFFER_SIZE);
creat_msg_q:
    //建立消息队列
    while((msgid = msgget((key_t)1234, 0666 | IPC_CREAT) == -1))
    {
        printf( "msgget failed with error: %d\n", errno);
        sleep(1);
    }
    //从队列中获取消息，直到遇到end消息为止
    while(1)
    {
        if((len = msgrcv(msgid, (void*)&data, MSG_Q_BUFFER_SIZE, msgtype, 0)) < 0)
        {
			printf("recreat msg q %d\r\n",__LINE__);
			goto creat_msg_q;
        }
        //hexdump(data.text,len);
        len = send(sockfd, data.text, len, 0);
        if(len < 1)
        {
        	printf("reconnect %d\r\n",__LINE__);
            pthread_exit(NULL);
        }
        memset(data.text,0,MSG_Q_BUFFER_SIZE);
    }
    //删除消息队列
    /*if(msgctl(msgid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "msgctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }*/
}

void *tcp_client_routin(void *data)
{
    int len;
    int sock_cli;
    struct sockaddr_in servaddr;
    int err;
	int flag,old_flag;
	int ret;
    pthread_t main_tid;
    lora_server_down_data_type datadown;
    struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
    uint8_t stringformat[256 * 2];
    uint8_t tx_data_buf[300];
connect:
    //定义IPV4的TCP连接的套接字描述符
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.149");
    servaddr.sin_port = htons(PORT);  //服务器端口
	flag |= O_NONBLOCK;
    old_flag = flag = fcntl(sock_cli, F_SETFL, O_NONBLOCK ); //将连接套接字设置为非阻塞。
    //连接服务器，成功返回0，错误返回-1
    ret = connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(ret != 0)
	{
		if(errno != EINPROGRESS) //connect返回错误。
		{
			//printf("connect failed\n");
		}
		//连接正在建立
		else
		{
			struct timeval tm;	//1.定时
			tm.tv_sec = 5;
			tm.tv_usec = 0;

			fd_set wset;
			FD_ZERO(&wset);
			FD_SET(sock_cli,&wset);
			//printf("selcet start\n");
			int res = select(sock_cli+1, NULL, &wset, NULL, &tm);
			//printf("select end\n");
			if(res <= 0)
			{
				//printf("res <= 0\n");
				close(sock_cli);
				sleep(1);
				goto connect;
				//return 2;
			}

			if(FD_ISSET(sock_cli,&wset))
			{
				//printf("test \n");
				int err = -1;
				socklen_t len = sizeof(int);

				if(getsockopt(sock_cli, SOL_SOCKET, SO_ERROR, &err, &len ) < 0) //两种错误处理方式
				{
					printf("errno :%d %s\n",errno, strerror(errno));
					close(sock_cli);
					sleep(1);
					goto connect;
					//return 3;
				}

				if(err)
				{
					//printf("connect faile\n");
					errno = err;
					close(sock_cli);
					//return 4;
					sleep(1);
					goto connect;
				}

				printf("connetct success\n");
			}

		}

	}

	fcntl(sock_cli, F_SETFL, old_flag); //最后恢复sock的阻塞属性。
#if 0
	{
        perror("connect");
    	close(sock_cli);
    	sleep(1);
		printf("reconnect %d\r\n",__LINE__);
        goto connect;//exit(1);
    }
#endif
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
        //printf("client receive:\n");
        len = recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        if(len < 1)
        {
            close(sock_cli);
			printf("reconnect %d\r\n",__LINE__);
            goto connect;
        }
        printf("%s\r\n",recvbuf);
        pragma = json_tokener_parse((const char *)recvbuf);
        json_object_object_get_ex(pragma,"DevEUI",&obj);
        memset(stringformat,0,256 * 2);
        strcpy(stringformat,json_object_get_string(obj));
        //printf("%s :%s\r\n", __func__,stringformat);
        Str2Hex( &stringformat[0],  datadown.DevEUI, 8 );
        hexdump(datadown.DevEUI,8);
        json_object_object_get_ex(pragma,"Port",&obj);
        datadown.fPort = json_object_get_int(obj);
        json_object_object_get_ex(pragma,"AckRequest",&obj);
        datadown.CtrlBits.AckRequest = json_object_get_boolean(obj);

        json_object_object_get_ex(pragma,"Ack",&obj);
        datadown.CtrlBits.Ack = json_object_get_boolean(obj);
        //printf("AckRequest = %d\r\nAck = %d\r\n", datadown.CtrlBits.AckRequest,datadown.CtrlBits.Ack);
        json_object_object_get_ex(pragma,"Data",&obj);
        memset(stringformat,0,256 * 2);
        strcpy(stringformat,json_object_get_string(obj));
        datadown.size = strlen(stringformat) / 2;
        //printf("%s\r\n", stringformat);
        Str2Hex( &stringformat[0],  &tx_data_buf[sizeof(lora_server_down_data_type)], datadown.size );
        hexdump(&tx_data_buf[sizeof(lora_server_down_data_type)],datadown.size);
        memcpy(tx_data_buf,&datadown,sizeof(lora_server_down_data_type));
        json_object_put(pragma);

        write(fd_cdev,tx_data_buf,sizeof(lora_server_down_data_type) + datadown.size);
        //memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    //sleep(1);
    }

    close(sock_cli);
    return 0;
}
