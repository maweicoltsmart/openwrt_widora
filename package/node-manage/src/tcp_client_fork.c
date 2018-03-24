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
#include "GatewayPragma.h"
#include "Server.h"
#include <errno.h>

//#define PORT  8890
uint8_t sendbuf[MSG_Q_BUFFER_SIZE];
uint8_t recvbuf[MSG_Q_BUFFER_SIZE * 100];
extern int fd_cdev;

void *client_send(void *arg)
{
    int sockfd = *(int*)arg;
    unsigned char buffer[MSG_Q_BUFFER_SIZE];
    int len;
    struct msg_st data;
    long int msgtype = 0;
    int msgid = -1;
    memset(data.text,0,MSG_Q_BUFFER_SIZE);
	printf("%s %d\r\n",__func__,__LINE__);
creat_msg_q:
    while((msgid = msgget((key_t)1234, 0666 | IPC_CREAT) == -1))
    {
        printf( "msgget failed with error: %d\n", errno);
        sleep(1);
    }
    while(1)
    {
        if((len = msgrcv(msgid, (void*)&data, MSG_Q_BUFFER_SIZE, msgtype, 0)) < 0)
        {
			printf("recreat msg q %d\r\n",__LINE__);
			goto creat_msg_q;
        }
        len = send(sockfd, data.text, len, 0);
		//printf("%s\r\n",data.text);
        if(len < 1)
        {
        	printf("reconnect %d\r\n",__LINE__);
            pthread_exit(NULL);
        }
        memset(data.text,0,MSG_Q_BUFFER_SIZE);
    }
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
    //lora_server_down_data_type datadown;
    struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
    uint8_t stringformat[256 * 2];
    uint8_t tx_data_buf[300];
    uint8_t *pstrchr;
	uint8_t writebuf[256 + sizeof(st_ServerMsgDown)];
	uint32_t sendlen;
	pst_ServerMsgDown pstServerMsgDown = (pst_ServerMsgDown)writebuf;
	//uint8_t loraport;
	uint8_t devEUI[8];
connect:
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(gateway_pragma.server_ip);
    servaddr.sin_port = htons(gateway_pragma.server_port);
	flag |= O_NONBLOCK;
    old_flag = flag = fcntl(sock_cli, F_SETFL, O_NONBLOCK );
    ret = connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(ret != 0)
	{
		if(errno != EINPROGRESS)
		{
			//printf("connect failed\n");
		}
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

	fcntl(sock_cli, F_SETFL, old_flag);
#if 0
	{
        perror("connect");
    	close(sock_cli);
    	sleep(1);
		printf("reconnect %d\r\n",__LINE__);
        goto connect;//exit(1);
    }
#endif
    printf("connect server(IP:%s).\r\n",gateway_pragma.server_ip);
    err = pthread_create(&main_tid, NULL, client_send, &sock_cli);
    while(1)
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        len = recv(sock_cli, recvbuf, sizeof(recvbuf) - 1,0);
        if(len < 1)
        {
            close(sock_cli);
			printf("reconnect %d\r\n",__LINE__);
            goto connect;
        }
        pstrchr = recvbuf;
        //printf("%s\r\n", recvbuf);
        while((pstrchr - recvbuf) < len)
        {
            pstrchr = strchr(pstrchr,'{');
            if(pstrchr == NULL)
            {
                break;
            }
            else
            {
                pstrchr ++;
            }
            //printf("%s\r\n",recvbuf);
            pragma = json_tokener_parse((const char *)(pstrchr - 1));
            if(pragma == NULL)
            {
                break;
            }

            json_object_object_get_ex(pragma,"FrameType",&obj);
            if(obj == NULL)
            {
                    break;
            }
			memset(stringformat,0,256 * 2);
            strcpy(stringformat,json_object_get_string(obj));
			if(strcmp(stringformat,"DownData") == 0)
			{
				pstServerMsgDown->enMsgDownFramType = en_MsgDownFramDataSend;
				pstServerMsgDown->Msg.stData2Node.payload = writebuf + sizeof(st_ServerMsgDown);
				json_object_object_get_ex(pragma,"NetAddr",&obj);
            	if(obj == NULL)
            	{
                    break;
            	}
            	pstServerMsgDown->Msg.stData2Node.DevAddr = json_object_get_int(obj);
				json_object_object_get_ex(pragma,"Port",&obj);
            	if(obj == NULL)
            	{
                    break;
            	}
            	pstServerMsgDown->Msg.stData2Node.fPort = json_object_get_int(obj);
            	if((pstServerMsgDown->Msg.stData2Node.fPort == 0) || (pstServerMsgDown->Msg.stData2Node.fPort > 223))
            	{
                	break;
            	}
				json_object_object_get_ex(pragma,"ConfirmRequest",&obj);
	            if(obj == NULL)
	            {
	                    break;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Confirm",&obj);
	            if(obj == NULL)
	            {
	                    break;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Data",&obj);
            	if(obj == NULL)
            	{
                    break;
            	}
            	memset(stringformat,0,256 * 2);
            	strcpy(stringformat,json_object_get_string(obj));
            	pstServerMsgDown->Msg.stData2Node.size = strlen(stringformat) / 2;
				sendlen = pstServerMsgDown->Msg.stData2Node.size + sizeof(st_ServerMsgDown);
            	Str2Hex( &stringformat[0],  pstServerMsgDown->Msg.stData2Node.payload, strlen(stringformat) );
			}
			else if(strcmp(stringformat,"DownConfirm") == 0)
			{
				pstServerMsgDown->enMsgDownFramType = en_MsgDownFramDataSend;
				json_object_object_get_ex(pragma,"NetAddr",&obj);
            	if(obj == NULL)
            	{
                    break;
            	}
            	pstServerMsgDown->Msg.stConfirm2Node.DevAddr = json_object_get_int(obj);
				sendlen = sizeof(st_ServerMsgDown);
			}
			else
			{
				break;
			}

            json_object_put(pragma);

            write(fd_cdev,writebuf,sendlen);
        }
    }

    close(sock_cli);
    return 0;
}
