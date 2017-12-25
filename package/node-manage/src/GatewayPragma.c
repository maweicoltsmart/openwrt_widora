#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <malloc.h>
#include<unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "GatewayPragma.h"
#include "nodedatabase.h"

#define GATEWAY_PRAGMA_FILE_PATH	"/usr/gatewaypragma.cfg"

gateway_pragma_t gateway_pragma;
int file;

void hex2str(uint8_t *byte,uint8_t *str,int len)
{
    char *ch = {"0123456789ABCDEF"};
    int loop;
    for(loop = 0;loop < len;loop++)
    {
        str[loop * 2] = ch[byte[loop] >> 4];
        str[loop * 2 + 1] = ch[byte[loop] & 0x0f];
    }
}

//字节流转换为十六进制字符串的另一种实现方式
void Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )
{
    int  i;
    char szTmp[3];

    for( i = 0; i < nSrcLen; i++ )
    {
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );
        memcpy( &sDest[i * 2], szTmp, 2 );
    }
    return ;
}

void GetGatewayPragma(gateway_pragma_t *gateway)
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
		hex2str(gateway_pragma.APPKEY,byte,16);
		json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
		memset(byte,0,100);
		hex2str(gateway_pragma.AppNonce,byte,3);
		json_object_object_add(pragma,"AppNonce",json_object_new_string(byte));
		memset(byte,0,100);
		hex2str(gateway_pragma.NetID,byte,3);
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
	json_object_object_get_ex(pragma,"radio",&array);
	for(i = 0; i < json_object_array_length(array); i++) {
		chip = json_object_array_get_idx(array, i);
		json_object_object_get_ex(chip,"index",&obj);
		tmp = json_object_get_int(obj);
		json_object_object_get_ex(chip,"channel",&obj);
		tmp = json_object_get_int(obj);
		json_object_object_get_ex(chip,"datarate",&obj);
		tmp = json_object_get_int(obj);
	}
	close(file);
	json_object_put(pragma);
	free(buf);
}