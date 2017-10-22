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
	
    //signal(SIGIO,node_event_fun);
	
    fd = open("/dev/lora_radio_1",O_RDWR);
    if (fd < 0)
    {
        printf("open error\n");
    }
	
	#if 0
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
#endif
	ret = pthread_create(&lora_1_handle, NULL, Radio_1_routin, &fd);
    /*fd = open("/dev/lora_radio_2",O_RDWR);
    if (fd < 0)
    {
        printf("open error\n");
    }
	ret = pthread_create(&lora_2_handle, NULL, Radio_2_routin, &fd);
	*/
	printf("%s,%d\r\n",__func__,__LINE__);
	ret = pthread_create(&tcp_client_handle, NULL, tcp_client_routin, &fd);
	printf("%s,%d\r\n",__func__,__LINE__);
	ret = pthread_create(&tcp_server_handle, NULL, tcp_server_routin, &fd);
	printf("%s,%d\r\n",__func__,__LINE__);
	while(1)
    {
        /* 为了测试，主函数里，什么也不做 */
        sleep(1000);
		printf("%s,%d\r\n",__func__,__LINE__);
    }
    return 0;
}  

