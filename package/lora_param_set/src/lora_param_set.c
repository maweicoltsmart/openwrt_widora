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

static int file;

char InputBuffer[4096];

int DecodeAndProcessData(char *input);    /*具体译码和处理数据，该函数代码略*/
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
    //printf("<p>hello test</p>");
    /*   从环境变量REQUEST_METHOD中得到METHOD属性值   */

    pRequestMethod = getenv("REQUEST_METHOD");
    if(pRequestMethod==NULL)
    {
                //printf("<p>request = null</p>");
        return   0;
    }
    if (strcasecmp(pRequestMethod,"POST")==0)
    {
        //printf("<p>OK the method is POST!\n</p>");
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
        //printf("<p>OK the method is GET!\n</p>");
        p = getenv("QUERY_STRING");     //从环境变量QUERY_STRING中得到Form数据
        if   (p!=NULL)
        {
            strncpy(InputBuffer,p,sizeof(InputBuffer));
                        //DecodeAndProcessData(InputBuffer);    //具体译码和处理数据，该函数代码略
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
            json_object_object_add(pragma,"localip",json_object_new_string("192.168.1.100"));
            json_object_object_add(pragma,"localport",json_object_new_string("32500"));

            json_object_object_add(pragma,"radio",array = json_object_new_array());
            for(loop = 0;loop < 3;loop++)
            {
                json_object_array_add(array,chip=json_object_new_object());
                json_object_object_add(chip,"index",json_object_new_int(loop));
                json_object_object_add(chip,"channel",json_object_new_int(loop));
                json_object_object_add(chip,"datarate",json_object_new_string("DR_0"));
            }
            memset(buf,0,4096);
            strcpy(buf,json_object_to_json_string(pragma));
            write(file,buf,strlen(buf));
            //printf("%s",buf);
            json_object_put(pragma);
            system("/etc/init.d/lora restart");
            //execl("/etc/init.d/lora","lora","restart",NULL);
        }
    //printf("Content-Type: application/json");
    //printf("%s\n\n","Content-Type:text/html;charset=utf-8");
    //printf("Content-type: text/html/n/n");
    //printf("");
                //printf("%s\n\n","Content-Type: application/json");
        printf("%s",buf);
        fflush(stdout);
        close(file);
        json_object_put(pragma);
        free(buf);
    }
        //printf("<HEAD><TITLE>Submitted OK</TITLE></HEAD>\n");//从stdout中输出返回信息
        //printf("<BODY>The information you supplied has been accepted.</BODY>\n");

    return   0;
}


int DecodeAndProcessData(char *input)    //具体译码和处理数据
{
    if(strlen(input) < 50)
        return 0;
    file = open(GATEWAY_PRAGMA_FILE_PATH, O_RDWR|O_CREAT);
    lseek(file,0,SEEK_SET);
    ftruncate(file,0);
    write(file,input,strlen(input));
    close(file);
    system("sync");
    system("/etc/init.d/lora restart");
    //usleep(100000);
        // 补充具体操作
    return 0;
}