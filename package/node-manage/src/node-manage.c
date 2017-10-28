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
#include "typedef.h"
#include "radio.h"
#include "sx1276.h"
#include "LoRaMac.h"
#include "typedef.h"
#include "Region.h"

void node_event_fun(int signum)
{
    unsigned char key_val;
    //read(fd,&key_val,1);
    printf("key_val = 0x%x\n",key_val);
}
int main(int argc ,char *argv[])
{
    int flag;
    int ret;
    int fd;
    pthread_t lora_1_handle,lora_2_handle,tcp_client_handle,tcp_server_handle;
	int msgid = -1;
	struct msg_st data;
	int len;
	//建立消息队列
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		printf("msgget failed with error: %d\r\n", errno);
	}
	fd = open("/dev/lora_radio",O_RDWR);
	if (fd < 0)
	{
		printf("open lora_radio error\r\n");
		return -1;
	}
	
    ret = pthread_create(&tcp_client_handle, NULL, tcp_client_routin, &fd);
    ret = pthread_create(&tcp_server_handle, NULL, tcp_server_routin, &fd);
#define RF_FREQUENCY                                433000000 // Hz
	SX1276SetChannel(0,fd,RF_FREQUENCY);
	SX1276SetChannel(1,fd,RF_FREQUENCY + FREQ_STEP * 10);
	while(1)
	{
		memset(radio2tcpbuffer,0,256);
		printf("%s read\r\n",__func__);
		if((len = read(fd,radio2tcpbuffer,256)) > 0)
		{
			printf("read one msg\r\n");
			data.msg_type = 1;	  //注意2
			memcpy(data.text, radio2tcpbuffer,len);
			//向队列发送数据
			if(msgsnd(msgid, (void*)&data, len, 0) == -1)
			{
				printf("msgsnd failed\r\n");
			}
			printf("%s:%s,%d\r\n",__func__,radio2tcpbuffer,len);
		}
		else
		{
			usleep(10000);
		}
	}
}

