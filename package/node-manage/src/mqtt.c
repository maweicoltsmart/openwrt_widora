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
extern int fd_cdev;
extern unsigned char strwifimacaddr[];
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

				pstServerMsgDown->enMsgDownFramType = en_MsgDownFramDataSend;
				pstServerMsgDown->Msg.stData2Node.payload = writebuf + sizeof(st_ServerMsgDown);
				json_object_object_get_ex(pragma,"NetAddr",&obj);
            	if((obj == NULL) && (!json_object_is_type(obj,json_type_int)))
            	{
            		json_object_put(pragma);
                    return;
            	}
            	pstServerMsgDown->Msg.stData2Node.DevAddr = json_object_get_int(obj);
				json_object_object_get_ex(pragma,"Port",&obj);
            	if((obj == NULL) && (!json_object_is_type(obj,json_type_int)))
            	{
            		json_object_put(pragma);
                    return;
            	}
            	pstServerMsgDown->Msg.stData2Node.fPort = json_object_get_int(obj);
            	if((pstServerMsgDown->Msg.stData2Node.fPort == 0) || (pstServerMsgDown->Msg.stData2Node.fPort > 223))
            	{
            		json_object_put(pragma);
                	return;
            	}
				json_object_object_get_ex(pragma,"ConfirmRequest",&obj);
	            if((obj == NULL) && (!json_object_is_type(obj,json_type_boolean)))
	            {
	            	json_object_put(pragma);
	            	return;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Confirm",&obj);
	            if((obj == NULL) && (!json_object_is_type(obj,json_type_boolean)))
	            {
	            	json_object_put(pragma);
	            	return;
	            }
	            pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack = json_object_get_boolean(obj);
				json_object_object_get_ex(pragma,"Data",&obj);
            	if((obj == NULL) && (!json_object_is_type(obj,json_type_string)))
            	{
            		json_object_put(pragma);
                    return;
            	}
                if(strlen(json_object_get_string(obj)) > 51 * 2)
                {
                	json_object_put(pragma);
                    return;
                }
            	memset(stringformat,0,256 * 2);
            	strcpy(stringformat,json_object_get_string(obj));
            	pstServerMsgDown->Msg.stData2Node.size = strlen(stringformat) / 2;
				sendlen = pstServerMsgDown->Msg.stData2Node.size + sizeof(st_ServerMsgDown);
            	Str2Hex( &stringformat[0],  pstServerMsgDown->Msg.stData2Node.payload, strlen(stringformat) );

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
    unsigned char topic[8 + 1 + 6 * 2 + 2 + 1 + 10 + 10 + 10] = {0};
    if(!result){
        memset(topic,0,sizeof(topic));

        /* Subscribe to broker information topics on successful connect. */
        /*if(!strcmp(gateway_pragma.username,"MJ-Modem") && !strcmp(gateway_pragma.password,"www.colt.xin") && !strcmp(gateway_pragma.server_ip,"101.132.97.241") && (gateway_pragma.server_port == 1883))
        {
            strcpy(topic,"LoRaWAN/Test/Down/");
        }
        else*/
        {
            strcpy(topic,"LoRaWAN/Down/");
        }
        strcat(topic,gateway_pragma.macaddress);
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

	uint8_t deveui[8 * 2 + 1] = {0};
	uint8_t senddata[1024] = {0};


    init_mqtt:
	//libmosquitto 库初始化
	mosquitto_lib_init();
	//创建mosquitto客户端
	mosq = mosquitto_new(NULL,session,NULL);
	if(!mosq){
		printf("create client failed..\n");
		mosquitto_lib_cleanup();
        sleep(1);
        goto init_mqtt;
		//return 0;
	}
	//设置回调函数，需要时可使用
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	//mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_username_pw_set(mosq,gateway_pragma.username,gateway_pragma.password);
	//printf("%s, %d\r\n",__func__,__LINE__);
	//连接服务器
	if(mosquitto_connect(mosq, gateway_pragma.server_ip, gateway_pragma.server_port, KEEP_ALIVE)){
		fprintf(stderr, "Unable to connect.\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        sleep(1);
        goto init_mqtt;
		//return 0;
	}
	//printf("%s, %d\r\n",__func__,__LINE__);
	//开启一个线程，在线程里不停的调用 mosquitto_loop() 来处理网络信息
	int loop = mosquitto_loop_start(mosq);
	if(loop != MOSQ_ERR_SUCCESS)
	{
		printf("mosquitto loop error\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        sleep(1);
        goto init_mqtt;
		//return 0;
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
                json_object_object_add(pragma,"Sn",json_object_new_int(pstServerMsgUp->Msg.stData2Server.sn));
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
				memcpy(deveui,stringformat,8 * 2);
				memset(stringformat,0,256 * 2);
				Hex2Str(pstServerMsgUp->Msg.stData2Server.AppEUI,stringformat,8);
				json_object_object_add(pragma,"NetAddr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.DevAddr));
				json_object_object_add(pragma,"AppEUI",json_object_new_string(stringformat));
				json_object_object_add(pragma,"Port",json_object_new_int(pstServerMsgUp->Msg.stData2Server.fPort));
				json_object_object_add(pragma,"ConfirmRequest",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.AckRequest));
				json_object_object_add(pragma,"Confirm",json_object_new_boolean(pstServerMsgUp->Msg.stData2Server.CtrlBits.Ack));
				json_object_object_add(pragma,"Battery",json_object_new_int(pstServerMsgUp->Msg.stData2Server.Battery));
				json_object_object_add(pragma,"Rssi",json_object_new_int(pstServerMsgUp->Msg.stData2Server.rssi));
				json_object_object_add(pragma,"Snr",json_object_new_int(pstServerMsgUp->Msg.stData2Server.snr));
				memset(stringformat,0,256 * 2);
                if(pstServerMsgUp->Msg.stData2Server.size)
                {
				    Hex2Str(pstServerMsgUp->Msg.stData2Server.payload,stringformat,pstServerMsgUp->Msg.stData2Server.size);
				    json_object_object_add(pragma,"Data",json_object_new_string(stringformat)); /* data that encoded into Base64 */
                }
                else
                {
                    json_object_object_add(pragma,"Data",json_object_new_string("")); /* data that encoded into Base64 */
                }
			}
			else
			{
				printf("%s ,%d\r\n",__func__,__LINE__);
			    return;
			}
			//printf("%s, %d\r\n",__func__,__LINE__);
			memset(senddata,0,sizeof(senddata));
			strcpy(senddata,json_object_to_json_string(pragma));
			/*发布消息*/
			unsigned char topic[8 + 1 + 6 * 2 + 1 + 8 * 2 + 1 + 2 + 1 + 10 + 10] = {0};
			//sprintf(topic,"%s,%s,%s,%s","LoRaWAN/",strmacaddr,"/","0123456789ABCDEF");
			/*if(!strcmp(gateway_pragma.username,"MJ-Modem") && !strcmp(gateway_pragma.password,"www.colt.xin") && !strcmp(gateway_pragma.server_ip,"101.132.97.241") && (gateway_pragma.server_port == 1883))
            {
                strcpy(topic,"LoRaWAN/Test/Up/");
            }
            else*/
            {
			    strcpy(topic,"LoRaWAN/Up/");
            }
			strcat(topic,gateway_pragma.macaddress);
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

