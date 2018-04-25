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
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <mosquitto.h>
#include <string.h>
#include <json-c/json.h>

#include "typedef.h"
#include "radio.h"
#include "typedef.h"
#include "LoRaDevOps.h"
#include "LoRaMacCrypto.h"
//#include "routin.h"
#include "mqtt.h"

unsigned char streth0macaddr[6 * 2 + 1] = {0};
unsigned char strwifimacaddr[6 * 2 + 1] = {0};
static bool resultfirewarecheck = false;
extern void serverpkgformat(void);
void *tcp_client_routin(void *data);
void* tcp_server_routin(void *data);

void node_event_fun(int signum)
{
    unsigned char key_val;
    //read(fd,&key_val,1);
    printf("key_val = 0x%x\n",key_val);
}
static uint8_t LoRaMacNwkSKey[] =
{
    'M', 'J', '-', 'M', 'o', 'd', 'e', 'm',
    'c', 'o', 'l', 't', '.', 'x', 'i', 'n',
};

static void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	//int len;
	struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
	uint8_t *pstrchr;

    if(message->payloadlen > 0)
	{
		pstrchr = message->payload;
        if(message->payload)
        {
            pragma = json_tokener_parse((const char *)pstrchr);
            if(pragma == NULL)
            {
                return;
            }

            json_object_object_get_ex(pragma,"Result",&obj);
            if(obj == NULL)
            {
            	return;
            }
            if(json_object_get_boolean(obj) == false)
            {
                exit(0);
            }
            else
            {
                resultfirewarecheck = true;
            }
       }
    }
}
static void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int i;
    unsigned char topic[8 + 1 + 6 * 2 + 2 + 1 + 10 + 20] = {0};
    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        strcpy(topic,"LoRaWAN/FirewareCheck/Down/");
        strcat(topic,strwifimacaddr);
        strcat(topic,"/#");
        mosquitto_subscribe(mosq, NULL, topic, 2);
        printf("topic = %s\r\n",topic);
    }else{
        fprintf(stderr, "Connect failed\n");
    }
}

void *mjcheckfirewarecopy_routin(void *data)
{
	int file;
    uint8_t buf[6 * 2 * 2 + 1];
	memset(buf,0,sizeof(buf));
	file = open("/usr/firewarecopycheck", O_RDWR);
	if(file < 0)
	{
	    printf("the fireware check file was not exist");
        exit(0);
	}
    
    lseek(file, 0, SEEK_SET);
    read(file,buf,strlen("firewarecopycheck"));
    if(!strcmp(buf,"firewarecopycheck"))
    {
        printf("init the fireware check file");
        LoRaMacPayloadEncrypt( (const uint8_t *)streth0macaddr, 6 * 2, (const uint8_t *)LoRaMacNwkSKey, 0xff, 0, 0xff, (uint8_t *)buf );
        LoRaMacPayloadEncrypt( (const uint8_t *)strwifimacaddr, 6 * 2, (const uint8_t *)LoRaMacNwkSKey, 0xff, 0, 0xff, (uint8_t *)buf + (6 * 2) );
        lseek(file, 0, SEEK_SET);
        write(file,buf,strlen(buf));
        close(file);
    }
    else
    {
        printf("decode\r\n");
        uint8_t decodebuf[6 * 2 * 2 + 1];
        lseek(file, 0, SEEK_SET);
        read(file,buf,sizeof(buf));
        LoRaMacPayloadDecrypt( (const uint8_t *)buf, 6 * 2, (const uint8_t *)LoRaMacNwkSKey, 0xff, 0, 0xff, (uint8_t *)decodebuf );
        LoRaMacPayloadDecrypt( (const uint8_t *)buf + (6 * 2), 6 * 2, (const uint8_t *)LoRaMacNwkSKey, 0xff, 0, 0xff, (uint8_t *)decodebuf + (6 * 2) );
        if((memcmp(decodebuf,streth0macaddr,6 * 2) == 0) && (memcmp((uint8_t *)decodebuf + (6 * 2),strwifimacaddr,6 * 2) == 0))
        {
            printf("fireware check succuss\r\n");
            close(file);
        }
        else
        {
            printf("fireware check file verify error\r\n");
            close(file);
            exit(0);
        }
    }
    #if 0
    struct json_object *pragma = NULL;
    struct mosquitto *mosq = NULL;
	uint8_t senddata[1024] = {0};
    bool session = true;
    init_mqtt:
	//libmosquitto ø‚≥ı ºªØ
	mosquitto_lib_init();
	//¥¥Ω®mosquittoøÕªß∂À
	mosq = mosquitto_new(NULL,session,NULL);
	if(!mosq){
		printf("create client failed..\n");
		mosquitto_lib_cleanup();
        sleep(10);
        goto init_mqtt;
		//return 0;
	}
	//…Ë÷√ªÿµ˜∫Ø ˝£¨–Ë“™ ±ø… π”√
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	//mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_username_pw_set(mosq,"MJ-Modem","www.colt.xin");
	//printf("%s, %d\r\n",__func__,__LINE__);
	//¡¨Ω”∑˛ŒÒ∆˜
	if(mosquitto_connect(mosq, "101.132.97.241", 1883, 60)){
		fprintf(stderr, "Unable to connect.\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        sleep(10);
        goto init_mqtt;
		//return 0;
	}
	//printf("%s, %d\r\n",__func__,__LINE__);
	//ø™∆Ù“ª∏ˆœﬂ≥Ã£¨‘⁄œﬂ≥Ã¿Ô≤ªÕ£µƒµ˜”√ mosquitto_loop() ¿¥¥¶¿ÌÕ¯¬Á–≈œ¢
	int loop = mosquitto_loop_start(mosq);
	if(loop != MOSQ_ERR_SUCCESS)
	{
		printf("mosquitto loop error\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        sleep(10);
        goto init_mqtt;
		//return 0;
	}
	//printf("%s, %d\r\n",__func__,__LINE__);
	while(resultfirewarecheck == false)
	{
	    pragma = json_object_new_object();
        json_object_object_add(pragma,"Eth0Addr",json_object_new_string(streth0macaddr));
        json_object_object_add(pragma,"WiFiAddr",json_object_new_string(strwifimacaddr));
		memset(senddata,0,sizeof(senddata));
		strcpy(senddata,json_object_to_json_string(pragma));
		/*∑¢≤ºœ˚œ¢*/
		unsigned char topic[8 + 1 + 6 * 2 + 1 + 8 * 2 + 1 + 2 + 1 + 10 + 20] = {0};
	    strcpy(topic,"LoRaWAN/FirewareCheck/Up/");
		strcat(topic,strwifimacaddr);
		strcat(topic,"/");			
		mosquitto_publish(mosq,NULL,topic,strlen(senddata)+1,senddata,0,0);
		json_object_put(pragma);
        sleep(60 * 60);
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
    #endif
}

int main(int argc ,char *argv[])
{
    int flag;
    int ret;
    int fd;
    pthread_t tcp_client_handle,tcp_server_handle,radio_routin_handle,mqtt_client_handle,mjcheckfirewarecopy_handle;
    int msgid = -1;
    //struct msg_st data;
    int len;

    char *device="br-lan";//"eth0"; //eth0 «Õ¯ø®…Ë±∏√˚
    unsigned char macaddr[6]; //ETH_ALEN£®6£© «MACµÿ÷∑≥§∂»
    struct ifreq req;
    int err,i;
    int s=socket(AF_INET,SOCK_DGRAM,0); //internet–≠“È◊Âµƒ ˝æ›±®¿‡–ÕÃ◊Ω”ø⁄
    strcpy(req.ifr_name,device); //Ω´…Ë±∏√˚◊˜Œ™ ‰»Î≤Œ ˝¥´»Î
    err=ioctl(s,SIOCGIFHWADDR,&req); //÷¥––»°MACµÿ÷∑≤Ÿ◊˜
    close(s);
    if(err != -1)
    {
         memcpy(macaddr,req.ifr_hwaddr.sa_data,6); //»° ‰≥ˆµƒMACµÿ÷∑
         sprintf(&strwifimacaddr[0 * 2],"%02X",macaddr[0]);
         sprintf(&strwifimacaddr[1 * 2],"%02X",macaddr[1]);
         sprintf(&strwifimacaddr[2 * 2],"%02X",macaddr[2]);
         sprintf(&strwifimacaddr[3 * 2],"%02X",macaddr[3]);
         sprintf(&strwifimacaddr[4 * 2],"%02X",macaddr[4]);
         sprintf(&strwifimacaddr[5 * 2],"%02X",macaddr[5]);
         printf("%s\r\n",strwifimacaddr);
    }
    device="eth0";//"eth0"; //eth0 «Õ¯ø®…Ë±∏√˚
    strcpy(req.ifr_name,device); //Ω´…Ë±∏√˚◊˜Œ™ ‰»Î≤Œ ˝¥´»Î
    err=ioctl(s,SIOCGIFHWADDR,&req); //÷¥––»°MACµÿ÷∑≤Ÿ◊˜
    close(s);
    if(err != -1)
    {
         memcpy(macaddr,req.ifr_hwaddr.sa_data,6); //»° ‰≥ˆµƒMACµÿ÷∑
         sprintf(&streth0macaddr[0 * 2],"%02X",macaddr[0]);
         sprintf(&streth0macaddr[1 * 2],"%02X",macaddr[1]);
         sprintf(&streth0macaddr[2 * 2],"%02X",macaddr[2]);
         sprintf(&streth0macaddr[3 * 2],"%02X",macaddr[3]);
         sprintf(&streth0macaddr[4 * 2],"%02X",macaddr[4]);
         sprintf(&streth0macaddr[5 * 2],"%02X",macaddr[5]);
         printf("%s\r\n",streth0macaddr);
    }
    serverpkgformat();
	LoRaMacInit();
    
    //ret = pthread_create(&radio_routin_handle, NULL, Radio_routin, &fd);
    //ret = pthread_create(&tcp_client_handle, NULL, tcp_client_routin, &fd);
    //ret = pthread_create(&tcp_server_handle, NULL, tcp_server_routin, &fd);
    ret = pthread_create(&mqtt_client_handle, NULL, mjmqtt_client_routin, &fd);
    ret = pthread_create(&mjcheckfirewarecopy_handle, NULL, mjcheckfirewarecopy_routin, &fd);
#define RF_FREQUENCY                                470000000 // Hz
    //SX1276SetChannel(0,fd,RF_FREQUENCY);
    //SX1276SetChannel(1,fd,RF_FREQUENCY + FREQ_STEP * 10);
    while(1)
    {
        sleep(100);
    }
#if 0
creat_msg_q:
    //Âª∫Á´ãÊ∂àÊÅØÈòüÂàó
    while((msgid = msgget((key_t)1234, 0666 | IPC_CREAT) == -1))
    {
        printf("msgget failed with error: %d\r\n", errno);
        sleep(1);
    }
    while(1)
    {
        memset(radio2tcpbuffer,0,256);
        if((len = read(fd,radio2tcpbuffer,256)) > 0)
        {
            data.msg_type = 1;    //Ê≥®ÊÑè2
            memcpy(data.text, radio2tcpbuffer,len);
            //ÂêëÈòüÂàóÂèëÈÄÅÊï∞ÊçÆ
            if(msgsnd(msgid, (void*)&data, len, 0) == -1)
            {
                printf("msgsnd failed\r\n");
                goto creat_msg_q;
            }
        }
        else
        {
            usleep(10000);
        }
    }
#endif
}

