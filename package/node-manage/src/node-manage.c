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

int fd;

void node_event_fun(int signum)  
{
    unsigned char key_val;
    read(fd,&key_val,1);
    printf("key_val = 0x%x\n",key_val);
}
int main(int argc ,char *argv[])
{
    int flag;
    int ret;
    pthread_t lora_handle;
	
    signal(SIGIO,node_event_fun);
	
    fd = open("/dev/lora_radio",O_RDWR);
    if (fd < 0)
    {
        printf("open error\n");
    }
    /* F_SETOWN:  Set the process ID
     *  告诉内核，发给谁
     */
    fcntl(fd, F_SETOWN, getpid());
	
    /*  F_GETFL :Read the file status flags 
     *  读出当前文件的状态 
     */
    flag = fcntl(fd,F_GETFL);
	
    /* F_SETFL: Set the file status flags to the value specified by arg 
     * int fcntl(int fd, int cmd, long arg); 
     * 修改当前文件的状态，添加异步通知功能 
     */  
    fcntl(fd,F_SETFL,flag | FASYNC);

	ret = pthread_create(&lora_handle, NULL, Radio_routin, NULL);
    while(1)
    {
        /* 为了测试，主函数里，什么也不做 */
        sleep(1000);
    }
    return 0;
}  

