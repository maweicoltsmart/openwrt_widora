#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <time.h>     //C语言的头文件

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

int main()
{
    time_t   now;         //实例化time_t结构
    struct   tm     *timenow;         //实例化tm结构指针
    unsigned char mybuf[50];
    int index = 0;
    unsigned char appeui[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    unsigned char deveui[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    unsigned char devaddr[4] ={0xab,0xcd,0xef,0x12};
    unsigned char devnonce[2] = {0xab,0x12};
    int wakeuptime = 1000;
    int rx1window = 1000;
    int rx2window = 1000;
    int i = 0;
    int loop  = 0;
    //先创建空对象
    struct json_object *json = json_object_new_object();
    //在对象上添加键值对
    //cJSON_AddStringToObject(json,"country","china");
    //添加数组
    struct json_object *array = NULL;
    json_object_object_add(json,"devices",array=json_object_new_array());
    for(loop = 0;loop < 20;loop ++)
    {
        //在数组上添加对象
        struct json_object *obj = NULL;
        json_object_array_add(array,obj=json_object_new_object());

        memset(mybuf,0,50);
        sprintf(mybuf,"%d",loop);
        json_object_object_add(obj,"index",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        Hex2Str(appeui,mybuf,8);
        json_object_object_add(obj,"appeui",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        Hex2Str(deveui,mybuf,8);
        json_object_object_add(obj,"deveui",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        Hex2Str(devaddr,mybuf,4);
        json_object_object_add(obj,"devaddr",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        Hex2Str(devnonce,mybuf,2);
        json_object_object_add(obj,"devnonce",json_object_new_string(mybuf));
        time(&now);
        //time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
        timenow   =   localtime(&now);
        //localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
        memset(mybuf,0,50);
        strcpy(mybuf,asctime(timenow));
        //printf("%d\n",strlen(mybuf));
        //mybuf[strlen(mybuf) - 1] = '\0';
        //printf("%d\n",strlen(mybuf));
        for(i = 49;i > 0;i --)
        {
            if(mybuf[i] == '\n')
                mybuf[i] = '\0';
        }
        json_object_object_add(obj,"lastcommunication",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        sprintf(mybuf,"%d",wakeuptime);
        json_object_object_add(obj,"wakeuptime",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        sprintf(mybuf,"%d",rx1window);
        json_object_object_add(obj,"rx1window",json_object_new_string(mybuf));
        memset(mybuf,0,50);
        sprintf(mybuf,"%d",rx2window);
        json_object_object_add(obj,"rx2window",json_object_new_string(mybuf));
    }
    printf("Content-type:text/html\n\n");
    char *buf = json_object_to_json_string(json);
    printf("%s",buf);
    fflush(stdout);
    json_object_put(json);
    return 0;
}