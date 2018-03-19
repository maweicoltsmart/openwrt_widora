#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <time.h>     //C语言的头文件
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include<unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#define GATEWAY_PRAGMA_FILE_PATH    "/overlay/upper/usr/gatewaypragma.cfg"
/*const gateway_pragma_t gateway_pragma = {
    .APPKEY = LORAWAN_APPLICATION_KEY,
    .AppNonce = {0x12,0x34,0x56},
    .NetID  ={0x78,0x9a,0xbc},
};*/

typedef struct
{
    uint8_t APPKEY[16];
    uint8_t AppNonce[3];
    uint8_t NetID[3];
    uint8_t server_ip[4];
    uint16_t server_port;
}gateway_pragma_t;

#define LORAWAN_APPLICATION_KEY                     { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }
#define GATEWAY_PRAGMA_FILE_PATH    "/usr/gatewaypragma.cfg"
const gateway_pragma_t gateway_pragma = {
    .APPKEY = LORAWAN_APPLICATION_KEY,
    .AppNonce = {0x12,0x34,0x56},
    .NetID  ={0x78,0x9a,0xbc},
};

char InputBuffer[4096];

int DecodeAndProcessData(char *input);    /*具体译码和处理数据，该函数代码略*/

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
    //sDest[i * 2] = '\0';
    return ;
}
int main(int argc, char*argv[])
{
    int   ContentLength;   /*数据长度*/
    int   x;
    int   i;
    char   *p;
    char   *pRequestMethod;     /*   METHOD属性值   */
    setvbuf(stdin,NULL,_IONBF,0);     /*关闭stdin的缓冲*/
    printf("Content-Type: application/json\n\n");     /*从stdout中输出，告诉Web服务器返回的信息类型*/
    printf("\n");                                           /*插入一个空行，结束头部信息*/
    /*   从环境变量REQUEST_METHOD中得到METHOD属性值   */

    pRequestMethod = getenv("REQUEST_METHOD");
    if(pRequestMethod==NULL)
    {
        return   0;
    }
    if (strcasecmp(pRequestMethod,"POST")==0)
    {
        p = getenv("CONTENT_LENGTH");     //从环境变量CONTENT_LENGTH中得到数据长度
        if (p!=NULL)
        {
            ContentLength = atoi(p);
        }
        else
        {
            ContentLength = 0;
        }
        if (ContentLength > sizeof(InputBuffer)-1)   {
            ContentLength = sizeof (InputBuffer) - 1;
        }

        i   =   0;
        while (i < ContentLength)
        {                         //从stdin中得到Form数据
            x  = fgetc(stdin);
            if (x==EOF)
                break;
            InputBuffer[i++] = x;
        }
        InputBuffer[i] = '\0';
        ContentLength   =   i;
        DecodeAndProcessData(InputBuffer);                 //具体译码和处理数据，该函数代码略
        //execl("/etc/init.d/lora","lora","restart",NULL);
    }
    else if (strcasecmp(pRequestMethod,"GET")==0)
    {
        p = getenv("QUERY_STRING");     //从环境变量QUERY_STRING中得到Form数据
        if   (p!=NULL)
        {
            strncpy(InputBuffer,p,sizeof(InputBuffer));
        }
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

            json_object_object_add(pragma,"NetType",json_object_new_string("Private"));

            memset(byte,0,100);
            Hex2Str(gateway_pragma.APPKEY,byte,16);
            json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
            memset(byte,0,100);
            Hex2Str(gateway_pragma.AppNonce,byte,3);
            json_object_object_add(pragma,"AppNonce",json_object_new_string(byte));
            memset(byte,0,100);
            Hex2Str(gateway_pragma.NetID,byte,3);
            json_object_object_add(pragma,"NetID",json_object_new_string(byte));
            json_object_object_add(pragma,"serverip",json_object_new_string("101.132.97.241"));
            json_object_object_add(pragma,"serverport",json_object_new_string("32500"));

            char hname[128];
            struct hostent *hent;
            int i;

            gethostname(hname, sizeof(hname));

            //hent = gethostent();
            hent = gethostbyname(hname);

            printf("hostname: %s/naddress list: ", hent->h_name);
            for(i = 0; hent->h_addr_list[i]; i++) {
                //printf("%s/t", inet_ntoa(*(struct in_addr*)(hent->h_addr_list[i])));
            }

            json_object_object_add(pragma,"localip",json_object_new_string(inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]))));
            json_object_object_add(pragma,"localport",json_object_new_string("32500"));

            json_object_object_add(pragma,"radio",array = json_object_new_array());
            const char* drname[] = {"DR_0","DR_1","DR_2","DR_3","DR_4","DR_5"};
            for(loop = 0;loop < 3;loop++)
            {
                json_object_array_add(array,chip=json_object_new_object());
                json_object_object_add(chip,"index",json_object_new_int(loop));
                json_object_object_add(chip,"channel",json_object_new_int(loop));
                json_object_object_add(chip,"datarate",json_object_new_string(drname[loop]));
            }
            json_object_to_file(GATEWAY_PRAGMA_FILE_PATH,pragma);
            system("/etc/init.d/lora restart");
        }

        printf("%s",json_object_to_json_string(pragma));
        fflush(stdout);
        json_object_put(pragma);
    }

    return   0;
}


int DecodeAndProcessData(char *input)    //具体译码和处理数据
{
    struct json_object *pragma = NULL;
    if(strlen(input) < 50)
        return 0;
    pragma = json_tokener_parse( (const char *)input );
    json_object_to_file(GATEWAY_PRAGMA_FILE_PATH,pragma);
    json_object_put(pragma);
    system("sync");
    system("/etc/init.d/lora restart");

    return 0;
}
