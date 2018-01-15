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

int file;

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
	uint8_t *buf = malloc(4096);

	memset(buf,0,4096);
	file = open(GATEWAY_PRAGMA_FILE_PATH, O_RDWR|O_CREAT);
	if(file < 0)
	{
            //printf("%s,%d,%d\r\n",__func__,__LINE__,errno);
	}
	lseek(file,0,SEEK_SET);
	len = read ( file,buf,4096) ;
	if(len == 0)
	{
            //return 0;
            //printf("%s,%d,%d\r\n",__func__,__LINE__,errno);
	}
        //printf("%s%d%s%d",__func__,__LINE__,buf,len);
	pragma = json_tokener_parse( (const char *)buf );
        //pragma = (struct json_object *)buf;
        //obj = json_object_object_get(pragma,"NetType");
	bool found;

	found = json_object_object_get_ex(pragma, "NetType", &obj);
	if(!found)
           //return json_object_get_string(val);
        //if((!obj))
	{
		lseek(file,0,SEEK_SET);
		ftruncate(file,0);
		memset(byte,0,100);
                    //sprintf(byte, "%d", errno);

        //fwrite("no nettype\r\n",strlen("no nettype\r\n"),1,file);
                    //fwrite(byte,1,strlen(byte),file);
		pragma = json_object_new_object();

        //if(gateway_pragma.nettype == 0)
		{

            json_object_object_add(pragma,"NetType",json_object_new_string("Private"));
		}
        //else
        //{
		//json_object_object_add(pragma,"NetType",json_object_new_string("Public"));
        //}
            /*memset(byte,0,100);
            sprintf(byte,"%032X",gateway_pragma.APPKEY);
            json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
            memset(byte,0,100);
            sprintf(byte,"%06X",gateway_pragma.AppNonce);
            json_object_object_add(pragma,"AppNonce",json_object_new_string(byte));
            memset(byte,0,100);
            sprintf(byte,"%06X",gateway_pragma.NetID);
            json_object_object_add(pragma,"NetID",json_object_new_string(byte));*/
		memset(byte,0,100);
		Hex2Str(gateway_pragma.APPKEY,byte,16);
		json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
		memset(byte,0,100);
		Hex2Str(gateway_pragma.AppNonce,byte,3);
		json_object_object_add(pragma,"AppNonce",json_object_new_string(byte));
		memset(byte,0,100);
		Hex2Str(gateway_pragma.NetID,byte,3);
		json_object_object_add(pragma,"NetID",json_object_new_string(byte));
		json_object_object_add(pragma,"serverip",json_object_new_string("192.168.1.100"));
		json_object_object_add(pragma,"serverport",json_object_new_string("32500"));
		char hname[128];
    	struct hostent *hent;
    	int i;

    	gethostname(hname, sizeof(hname));

    	//hent = gethostent();
    	hent = gethostbyname(hname);

    	printf("hostname: %s/naddress list: ", hent->h_name);
    	for(i = 0; hent->h_addr_list[i]; i++) {
        	printf("%s/t", inet_ntoa(*(struct in_addr*)(hent->h_addr_list[i])));
    	}

		json_object_object_add(pragma,"localip",json_object_new_string(inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]))));
		json_object_object_add(pragma,"localport",json_object_new_string("32500"));

		json_object_object_add(pragma,"radio",array = json_object_new_array());
		const char* drname[3] = {"DR_0","DR_3","DR_5"};
		for(loop = 0;loop < 3;loop++)
		{
			json_object_array_add(array,chip=json_object_new_object());
			json_object_object_add(chip,"index",json_object_new_int(loop));
			json_object_object_add(chip,"channel",json_object_new_int(loop));
			json_object_object_add(chip,"datarate",json_object_new_string(drname[loop]));
		}
		memset(buf,0,4096);
		strcpy(buf,json_object_to_json_string(pragma));
		write(file,buf,strlen(buf));
            //printf("%s",buf);
		json_object_put(pragma);
	}
	pragma = json_tokener_parse((const char *)buf);
	const char *nettype[2] = {"\"Private\"","\"Public\""};

	json_object_object_get_ex(pragma,"NetType",&obj);
	memset(byte,0,100);
	strcpy(byte,json_object_to_json_string(obj));
	if(strcmp(byte,nettype[0]) == 0)
	{
		gateway_pragma.NetType = 0;
	}
	else
	{
		gateway_pragma.NetType = 1;
	}

	json_object_object_get_ex(pragma,"radio",&array);
	char dataratename[10] = {0};
	for(i = 0; i < json_object_array_length(array); i++) {
		chip = json_object_array_get_idx(array, i);
		json_object_object_get_ex(chip,"index",&obj);
		gateway_pragma.radio[i].index = json_object_get_int(obj);
		json_object_object_get_ex(chip,"channel",&obj);
		gateway_pragma.radio[i].channel = json_object_get_int(obj);
		json_object_object_get_ex(chip,"datarate",&obj);

		strcpy(dataratename,json_object_to_json_string(obj));
		//printf("datarate is : %c\r\n",dataratename[4]);
		gateway_pragma.radio[i].datarate = 12 - (dataratename[4] - '0');
	}
	close(file);
	json_object_put(pragma);
	free(buf);
}