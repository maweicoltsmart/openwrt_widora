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
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <errno.h>
#include <json-c/json.h>

#include "typedef.h"
#include "LoRaDevOps.h"

#define PORT  8890
#define QUEUE_SIZE   10
extern int fd_cdev;

void *server_send(void *arg)
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
            printf("msgrcv failed with errno: %d\n", errno);
            goto creat_msg_q;
        }
        //printf("%s\r\n",data.text);
        len = send(sockfd, data.text, len, 0);
        if(len < 1)
        {
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

void str_echo(int sockfd)
{
    char buffer[MSG_Q_BUFFER_SIZE];
    int len;
    int err;
    //uint8_t *tmp;
    //int index;
    pthread_t main_tid;
    lora_server_down_data_type datadown;
    struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
    uint8_t stringformat[256 * 2];
    uint8_t tx_data_buf[300];
    err = pthread_create(&main_tid, NULL, server_send, &sockfd); //创建线程
    pid_t pid = getpid();
    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        len = recv(sockfd, buffer, sizeof(buffer),0);
        if(len < 1)
        {
            printf("child process: %d exited.\n",pid);
            break;
        }
        if(strcmp(buffer,"exit\n")==0)
        {
            printf("child process: %d exited.\n",pid);
            break;
        }
        //printf("pid:%d %dreceive:\r\n",pid,len);
        //fputs(buffer, stdout);
        pragma = json_tokener_parse((const char *)buffer);
        json_object_object_get_ex(pragma,"DevEUI",&obj);
        memset(stringformat,0,256 * 2);
        strcpy(stringformat,json_object_get_string(obj));
        //printf("%s : %s\r\n", __func__,stringformat);
        Str2Hex( &stringformat[0],  datadown.DevEUI, 8 );
        //hexdump(datadown.DevEUI,8);
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
        /*len = send(sockfd, buffer, len, 0);
    if(len < 1)
        {
            printf("child process: %d exited.\n",pid);
        }*/
    }
    close(sockfd);
}

void* tcp_server_routin(void *data)
{
    //定义IPV4的TCP连接的套接字描述符
    int server_sockfd = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(PORT);
    int opt = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    {
        perror("Server setsockopt failed");
        return 0;
    }
    //bind成功返回0，出错返回-1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1)
    {
        perror("bind");
        exit(1);//1为异常退出
    }
    printf("bind success.\n");

    //listen成功返回0，出错返回-1，允许同时帧听的连接数为QUEUE_SIZE
    while(listen(server_sockfd,QUEUE_SIZE) == -1)
    {
        perror("listen");
        sleep(1);//exit(1);
    }
    printf("listen success.\n");

    for(;;)
    {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        //进程阻塞在accept上，成功返回非负描述字，出错返回-1
        int conn = accept(server_sockfd, (struct sockaddr*)&client_addr,&length);
        if(conn<0)
        {
            perror("connect");
            continue;//exit(1);
        }
        printf("new client accepted.\n");

        pid_t childid;
        if(childid=fork()==0)//子进程
        {
            printf("child process: %d created.\n", getpid());
            close(server_sockfd);//在子进程中关闭监听
            str_echo(conn);//处理监听的连接
            exit(0);
        }
    }

    printf("closed.\n");
    close(server_sockfd);
    return 0;
}
