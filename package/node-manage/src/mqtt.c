#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <string.h>
#include <json-c/json.h>
#include <errno.h>

#include "mqtt.h"
#include "typedef.h"
#include "LoRaDevOps.h"
#include "GatewayPragma.h"
#include "Server.h"

//#define HOST "101.132.97.241"
//#define PORT  32500
#define KEEP_ALIVE 60
#define MSG_MAX_SIZE  512

bool session = true;
unsigned char strmacaddr[6 * 2 + 1] = {0};
extern int fd_cdev;

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	//int len;
	struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
	uint8_t *pstrchr;
	uint8_t writebuf[256 + sizeof(st_ServerMsgDown)];
	uint32_t sendlen;
	uint8_t stringformat[256 * 2];

	//printf("%s ,%d\r\n",__func__,__LINE__);
	pst_ServerMsgDown pstServerMsgDown = (pst_ServerMsgDown)writebuf;
    if(message->payloadlen > 0)
	{
        //printf("%s %s\r\n", message->topic, message->payload);
		pstrchr = message->payload;
        //printf("%s\r\n", message->payload);
        if(message->payload)
        {
            pragma = json_tokener_parse((const char *)pstrchr);
            if(pragma == NULL)
            {
                return;
            }

            json_object_object_get_ex(pragma,"FrameType",&obj);
            if(obj == NULL)
            {
            	return;
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
                    return;
            	}
            	pstServerMsgDown->Msg.stData2Node.DevAddr = json_object_get_int(obj);
				json_object_object_get_ex(pragma,"Port",&obj);
            	if(obj == NULL)
            	{
                    return;
            	}
            	pstServerMsgDown->Msg.stData2Node.fPort = json_object_get_int(obj);
            	if((pstServerMsgDown->Msg.stData2Node.fPort == 0) || (pstServerMsgDown->Msg.stData2Node.fPort > 223))
            	{
                	return;
            	}
				json_object_object_get_ex(pragma,"ConfirmRequest",&obj);
	            if(obj == NULL)
	            {
	            	return;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Confirm",&obj);
	            if(obj == NULL)
	            {
	            	return;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Data",&obj);
            	if(obj == NULL)
            	{
                    return;
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
                    return;
            	}
            	pstServerMsgDown->Msg.stConfirm2Node.DevAddr = json_object_get_int(obj);
				sendlen = sizeof(st_ServerMsgDown);
			}
			else
			{
				return;
			}

            json_object_put(pragma);

            write(fd_cdev,writebuf,sendlen);
        }
    }else{
        printf("%s (null)\n", message->topic);
    }
    fflush(stdout);
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int i;
    unsigned char topic[8 + 1 + 6 * 2 + 2 + 1 + 10] = {0};
    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        strcpy(topic,"LoRaWAN/Down/");
        strcat(topic,strmacaddr);
        strcat(topic,"/#");
        mosquitto_subscribe(mosq, NULL, topic, 2);
        printf("topic = %s\r\n",topic);
    }else{
        fprintf(stderr, "Connect failed\n");
    }
}
void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    int i;
    printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(i=1; i<qos_count; i++){
        printf(", %d", granted_qos[i]);
    }
    printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Pring all log messages regardless of level. */
    printf("%s\n", str);
}

void *mjmqtt_client_routin(void *data)
{
	uint8_t stringformat[256 * 2];
	int len;
    struct json_object *pragma = NULL;
	uint8_t readbuffer[256 + sizeof(st_ServerMsgUp)];
	pst_ServerMsgUp pstServerMsgUp = (pst_ServerMsgUp)readbuffer;
    struct mosquitto *mosq = NULL;
    char *device="br-lan";//"eth0"; //eth0是网卡设备名
    unsigned char macaddr[6]; //ETH_ALEN（6）是MAC地址长度
    struct ifreq req;
    int err,i;
	uint8_t deveui[8 * 2 + 1] = {0};
	uint8_t senddata[1024] = {0};

    int s=socket(AF_INET,SOCK_DGRAM,0); //internet协议族的数据报类型套接口
    strcpy(req.ifr_name,device); //将设备名作为输入参数传入
    err=ioctl(s,SIOCGIFHWADDR,&req); //执行取MAC地址操作
    close(s);
    if(err != -1)
    {
         memcpy(macaddr,req.ifr_hwaddr.sa_data,6); //取输出的MAC地址
         sprintf(&strmacaddr[0 * 2],"%02X",macaddr[0]);
         sprintf(&strmacaddr[1 * 2],"%02X",macaddr[1]);
         sprintf(&strmacaddr[2 * 2],"%02X",macaddr[2]);
         sprintf(&strmacaddr[3 * 2],"%02X",macaddr[3]);
         sprintf(&strmacaddr[4 * 2],"%02X",macaddr[4]);
         sprintf(&strmacaddr[5 * 2],"%02X",macaddr[5]);
         printf("%s\r\n",strmacaddr);
	}
    init_mqtt:
	//libmosquitto 库初始化
	mosquitto_lib_init();
	//创建mosquitto客户端
	mosq = mosquitto_new(NULL,session,NULL);
	if(!mosq){
		printf("create client failed..\n");
		mosquitto_lib_cleanup();
		return 0;
	}
	//设置回调函数，需要时可使用
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	//mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	//printf("%s, %d\r\n",__func__,__LINE__);
	//连接服务器
	if(mosquitto_connect(mosq, gateway_pragma.server_ip, gateway_pragma.server_port, KEEP_ALIVE)){
		fprintf(stderr, "Unable to connect.\n");
		return 0;
	}
	//printf("%s, %d\r\n",__func__,__LINE__);
	//开启一个线程，在线程里不停的调用 mosquitto_loop() 来处理网络信息
	int loop = mosquitto_loop_start(mosq);
	if(loop != MOSQ_ERR_SUCCESS)
	{
		printf("mosquitto loop error\n");
		return 0;
	}
	//printf("%s, %d\r\n",__func__,__LINE__);
	while(1)
	{
		//printf("%s, %d\r\n",__func__,__LINE__);
		if((len = read(fd_cdev,readbuffer,sizeof(readbuffer))) > 0)
		{
			//printf("%s, %d\r\n",__func__,__LINE__);
			pragma = json_object_new_object();
			if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramDataReceive)
			{
				//printf("%s, %d\r\n",__func__,__LINE__);
				pstServerMsgUp->Msg.stData2Server.payload = &readbuffer[sizeof(st_ServerMsgUp)];
				json_object_object_add(pragma,"FrameType",json_object_new_string("UpData"));
				switch(pstServerMsgUp->Msg.stData2Server.ClassType)
				{
					case 0:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class A"));
						break;
					case 1:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class B"));
						break;
					case 2:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class C"));
						break;
				}
				memset(stringformat,0,256 * 2);
				Hex2Str(pstServerMsgUp->Msg.stData2Server.DevEUI,stringformat,8);
				json_object_object_add(pragma,"DevEUI",json_object_new_string(stringformat));
				memcpy(deveui,stringformat,8 * 2);
				memset(stringformat,0,256 * 2);
				Hex2Str(pstServerMsgUp->Msg.stData2Server.AppEUI,stringformat,8);
				json_object_object_add(pragma,"NetAddr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.DevAddr));
				json_object_object_add(pragma,"AppEUI",json_object_new_string(stringformat));
				json_object_object_add(pragma,"Port",json_object_new_int(pstServerMsgUp->Msg.stData2Server.fPort));
				json_object_object_add(pragma,"ConfirmRequest",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest));
				//json_object_object_add(pragma,"Confirm",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack));
				json_object_object_add(pragma,"Battery",json_object_new_int(pstServerMsgUp->Msg.stData2Server.Battery));
				json_object_object_add(pragma,"Rssi",json_object_new_int(pstServerMsgUp->Msg.stData2Server.rssi));
				json_object_object_add(pragma,"Snr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.snr));
				memset(stringformat,0,256 * 2);
				Hex2Str(pstServerMsgUp->Msg.stData2Server.payload,stringformat,pstServerMsgUp->Msg.stData2Server.size);
				json_object_object_add(pragma,"Data",json_object_new_string(stringformat)); /* data that encoded into Base64 */
			}
			else if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramConfirm)
			{
				json_object_object_add(pragma,"FrameType",json_object_new_string("UpConfirm"));
				switch(pstServerMsgUp->Msg.stConfirm2Server.ClassType)
				{
					case 0:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class A"));
						break;
					case 1:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class B"));
						break;
					case 2:
						json_object_object_add(pragma,"NodeType",json_object_new_string("Class C"));
						break;
				}
				memset(stringformat,0,256 * 2);
				Hex2Str(pstServerMsgUp->Msg.stConfirm2Server.DevEUI,stringformat,8);
				json_object_object_add(pragma,"DevEUI",json_object_new_string(stringformat));
				json_object_object_add(pragma,"NetAddr",json_object_new_int(pstServerMsgUp->Msg.stConfirm2Server.DevAddr));
				switch(pstServerMsgUp->Msg.stConfirm2Server.enConfirm2Server)
				{
					case en_Confirm2ServerSuccess:
						json_object_object_add(pragma,"Result",json_object_new_string("Trasmit Success"));
						break;
					case en_Confirm2ServerRadioBusy:
						json_object_object_add(pragma,"Result",json_object_new_string("Radio Busy"));
						break;
					case en_Confirm2ServerOffline:
						json_object_object_add(pragma,"Result",json_object_new_string("Not Active"));
						break;
					case en_Confirm2ServerTooLong:
						json_object_object_add(pragma,"Result",json_object_new_string("Data Too Long"));
						break;
					case en_Confirm2ServerPortNotAllow:
						json_object_object_add(pragma,"Result",json_object_new_string("Port Error"));
						break;
					case en_Confirm2ServerInLastDutyCycle:
						json_object_object_add(pragma,"Result",json_object_new_string("Last Pkg Is Sending"));
						break;
					case en_Confirm2ServerNodeNoAck:
						json_object_object_add(pragma,"Result",json_object_new_string("Node Have No Ack"));
						break;
					case en_Confirm2ServerNodeNotOnRxWindow:
						json_object_object_add(pragma,"Result",json_object_new_string("Rx Windows off"));
						break;
					default:
						json_object_object_add(pragma,"Result",json_object_new_string("Unkown Fault"));
						break;
				}
				//json_object_object_add(pragma,"Rssi",json_object_new_int(pstServerMsgUp->Msg.stConfirm2Server.rssi));
				//json_object_object_add(pragma,"Snr",json_object_new_double(pstServerMsgUp->Msg.stConfirm2Server.snr));
			}
			else
			{
				printf("%s ,%d\r\n",__func__,__LINE__);
			}
			//printf("%s, %d\r\n",__func__,__LINE__);
			memset(senddata,0,sizeof(senddata));
			strcpy(senddata,json_object_to_json_string(pragma));
			/*发布消息*/
			unsigned char topic[8 + 1 + 6 * 2 + 1 + 8 * 2 + 1 + 2 + 1 + 10] = {0};
			//sprintf(topic,"%s,%s,%s,%s","LoRaWAN/",strmacaddr,"/","0123456789ABCDEF");
			strcpy(topic,"LoRaWAN/Up/");
			strcat(topic,strmacaddr);
			strcat(topic,"/");
			strcat(topic,deveui);
			
			//printf("topic = %s\r\n",topic);
			mosquitto_publish(mosq,NULL,topic,strlen(senddata)+1,senddata,0,0);
			json_object_put(pragma);
		}
		else
		{
			usleep(100000);
		}
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
    goto init_mqtt;
	return 0;
}

