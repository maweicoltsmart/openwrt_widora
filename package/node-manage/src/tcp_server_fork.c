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
#include "typedef.h"

#define PORT  8890
#define QUEUE_SIZE   10
#define BUFFER_SIZE 1024

static int fd;
void *server_send(void *arg)
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
        if(msgrcv(msgid, (void*)&data, BUFFER_SIZE, msgtype, 0) == -1)
        {
            printf("msgrcv failed with errno: %d\n", errno);
            goto creat_msg_q;
        }
        len = send(sockfd, data.text, strlen(data.text), 0);
        if(len < 1)
        {
            pthread_exit(NULL);
        }
        memset(data.text,0,BUFFER_SIZE);
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
    char buffer[BUFFER_SIZE];
    int len;
    int err;
    uint8_t *tmp;
    int index;
    pthread_t main_tid;
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
        printf("pid:%d %dreceive:\r\n",pid,len);
        fputs(buffer, stdout);
        tmp = malloc(10 + len);
        tmp[index ++] = 0x00;   // address
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x00;
        tmp[index ++] = 0x01;   // port
        tmp[index ++] = len;    // len
        memcpy(&tmp[index],buffer,len);

        write(fd,tmp,10 + len);
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
    fd = *(int*)data;
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
