#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "GatewayPragma.h"
#include "nodedatabase.h"
#include "utilities.h"

#define LORAWAN_APPLICATION_KEY                     { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }
#define GATEWAY_PRAGMA_FILE_PATH    "/usr/gatewaypragma.cfg"
gateway_pragma_t gateway_pragma = {
    .APPKEY = LORAWAN_APPLICATION_KEY,
    .AppNonce = {0x12,0x34,0x56},
    .NetID  ={0x78,0x9a,0xbc},
};

#define VERSION_STR   "VERSION 1.0 ("__DATE__" "__TIME__")"

extern unsigned char streth0macaddr[];
extern unsigned char strwifimacaddr[];

void GetGatewayPragma(void)
{
    int len;
    int32_t tmp;
    struct json_object *pragma = NULL;
    struct json_object *chip = NULL;
    struct json_object *obj = NULL;
    struct json_object *array = NULL;
    int i = 0;
    int loop = 0;
    uint8_t byte[100];
    bool found;

    pragma = json_object_from_file(GATEWAY_PRAGMA_FILE_PATH);
    if(pragma == NULL)
    {
        found = false;
    }
    else
    {
        found = json_object_object_get_ex(pragma, "NetType", &obj);
    }

    if(!found)
    {
        pragma = json_object_new_object();
        json_object_object_add(pragma,"SoftWareVersion",json_object_new_string(VERSION_STR));
        json_object_object_add(pragma,"MacAddress",json_object_new_string(streth0macaddr));
        json_object_object_add(pragma,"UserName",json_object_new_string("MJ-LoRaWAN-Gateway"));
        json_object_object_add(pragma,"Password",json_object_new_string("www.coltsmart.com"));
        json_object_object_add(pragma,"NetType",json_object_new_string("MQTT"));
        json_object_object_add(pragma,"SlaveID",json_object_new_int(1));
        json_object_object_add(pragma,"Baud",json_object_new_int(115200));
        json_object_object_add(pragma,"Parity",json_object_new_string("8N1"));
        
        memset(byte,0,100);
        reverse(gateway_pragma.APPKEY,gateway_pragma.APPKEY,16);
        Hex2Str(gateway_pragma.APPKEY,byte,16);
        json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
        memset(byte,0,100);
        Hex2Str(gateway_pragma.NetID,byte,3);
        json_object_object_add(pragma,"NetID",json_object_new_string(byte));
        json_object_object_add(pragma,"serverip",json_object_new_string("39.100.123.34"));
        json_object_object_add(pragma,"serverport",json_object_new_string("1883"));

        json_object_object_add(pragma,"radio",array = json_object_new_array());
        const char* drname[] = {"DR_0","DR_1","DR_2","DR_3","DR_4","DR_5"};
        for(loop = 0;loop < 4;loop++)
        {
            json_object_array_add(array,chip=json_object_new_object());
            json_object_object_add(chip,"index",json_object_new_int(loop));
            printf("index = %d\n", loop);
            json_object_object_add(chip,"channel",json_object_new_int(loop * 6));
            json_object_object_add(chip,"datarate",json_object_new_string(drname[5]));
        }
        json_object_to_file(GATEWAY_PRAGMA_FILE_PATH,pragma);
    }

    json_object_object_get_ex(pragma,"SoftWareVersion",&obj);
    memset(gateway_pragma.softversion,0,64);
    strcpy(gateway_pragma.softversion,json_object_get_string(obj));
    json_object_object_get_ex(pragma,"MacAddress",&obj);
    memset(gateway_pragma.macaddress,0,6 * 2 + 1);
    strcpy(gateway_pragma.macaddress,json_object_get_string(obj));
    json_object_object_get_ex(pragma,"UserName",&obj);
    memset(gateway_pragma.username,0,16 + 1);
    strcpy(gateway_pragma.username,json_object_get_string(obj));
    json_object_object_get_ex(pragma,"Password",&obj);
    memset(gateway_pragma.password,0,16 + 1);
    strcpy(gateway_pragma.password,json_object_get_string(obj));
    
    const char *nettype[2] = {"Modbus","MQTT"};

    json_object_object_get_ex(pragma,"NetType",&obj);
    memset(byte,0,100);
    strcpy(byte,json_object_get_string(obj));
    
    if(strcmp(byte,nettype[0]) == 0)
    {
        gateway_pragma.NetType = 0;
    }
    else
    {
        gateway_pragma.NetType = 1;
    }
    json_object_object_get_ex(pragma,"SlaveID",&obj);
    gateway_pragma.slaveid = json_object_get_int(obj);
    json_object_object_get_ex(pragma,"Baud",&obj);
    gateway_pragma.baud = json_object_get_int(obj);
    json_object_object_get_ex(pragma,"Parity",&obj);
    memset(byte,0,100);
    strcpy(byte,json_object_get_string(obj));
    gateway_pragma.parity = byte[1];
    
	json_object_object_get_ex(pragma,"APPKEY",&obj);
	memset(byte,0,100);
    strcpy(byte,json_object_get_string(obj));
	Str2Hex(byte,gateway_pragma.APPKEY,2 * 16);
    reverse(gateway_pragma.APPKEY,gateway_pragma.APPKEY,16);

	json_object_object_get_ex(pragma,"NetID",&obj);
	memset(byte,0,100);
    strcpy(byte,json_object_get_string(obj));
	Str2Hex(byte,gateway_pragma.NetID,2 * 3);

	json_object_object_get_ex(pragma,"serverip",&obj);
	memset(gateway_pragma.server_ip,0,MAX_IP_STRING_LENTH);
    strcpy(gateway_pragma.server_ip,json_object_get_string(obj));

	json_object_object_get_ex(pragma,"serverport",&obj);
	memset(byte,0,100);
    strcpy(byte,json_object_get_string(obj));
	gateway_pragma.server_port = atoi(byte);

    json_object_object_get_ex(pragma,"radio",&array);
    char dataratename[10] = {0};
    for(i = 0; i < json_object_array_length(array); i++) {
        chip = json_object_array_get_idx(array, i);
        json_object_object_get_ex(chip,"index",&obj);
        gateway_pragma.radio[i].index = json_object_get_int(obj);
        json_object_object_get_ex(chip,"channel",&obj);
        gateway_pragma.radio[i].channel = json_object_get_int(obj);
        json_object_object_get_ex(chip,"datarate",&obj);

        strcpy(dataratename,json_object_get_string(obj));
        gateway_pragma.radio[i].datarate = 12 - (dataratename[3] - '0');
        printf("channel = %d; datarate = %d\r\n",gateway_pragma.radio[i].channel,gateway_pragma.radio[i].datarate);
    }
    /*gateway_pragma.radio[2].channel = gateway_pragma.radio[0].channel;
    gateway_pragma.radio[2].datarate = gateway_pragma.radio[0].datarate;
    gateway_pragma.radio[3].channel = gateway_pragma.radio[1].channel;
    gateway_pragma.radio[3].datarate = gateway_pragma.radio[1].datarate;*/
    json_object_put(pragma);
}
