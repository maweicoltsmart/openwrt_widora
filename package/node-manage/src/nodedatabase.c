#include "nodedatabase.h"

gateway_pragma_t gateway_pragma;
node_pragma_t nodebase_node_pragma[MAX_NODE];
static int16_t node_cnt = 0;

void hexdump(const unsigned char *buf, const int num)
{
    int i;
    for(i = 0; i < num; i++)
    {
        printf("%02X ", buf[i]);
        /*if ((i+1)%8 == 0)
            printf("\n");*/
    }
    printf("\r\n");
    return;
}

int16_t database_node_join(node_join_info_t* node)
{
    uint16_t loop;
    time_t   now;
    printf("%s, %d\r\n",__func__,__LINE__);
    for(loop = 0;loop < node_cnt;loop ++)
    {
        if(!memcmp(node->DevEUI,nodebase_node_pragma[loop].DevEUI,8)){
            LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, gateway_pragma.AppNonce, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
            nodebase_node_pragma[loop].DevNonce = node->DevNonce;
            time(&now);
            //time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
            nodebase_node_pragma[loop].LastComunication   =   localtime(&now);
            *(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
            node_cnt ++;
            printf("APPEUI: 0x");
            hexdump(node->APPEUI,8);
            printf("DevEUI: 0x");
            hexdump(node->DevEUI,8);
            printf("DevNonce: 0x");
            hexdump(&node->DevNonce,2);
            printf("APPKEY: 0x");
            hexdump(nodebase_node_pragma[loop].AppSKey,16);
            printf("NwkSKey: 0x");
            hexdump(nodebase_node_pragma[loop].NwkSKey,16);
            return loop;
        }
    }
    if(loop < MAX_NODE){
        nodebase_node_pragma[loop].DevNonce = node->DevNonce;
        memcpy(nodebase_node_pragma[loop].APPEUI, node->APPEUI, 8);
        memcpy(nodebase_node_pragma[loop].DevEUI, node->DevEUI, 8);
        time(&now);
        //time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
        nodebase_node_pragma[loop].LastComunication   =   localtime(&now);
        LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, gateway_pragma.AppNonce, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
        *(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
        printf("APPEUI: 0x");
        hexdump(node->APPEUI,8);
        printf("DevEUI: 0x");
        hexdump(node->DevEUI,8);
        printf("DevNonce: 0x");
        hexdump(&node->DevNonce,2);
        printf("APPKEY: 0x");
        hexdump(nodebase_node_pragma[loop].AppSKey,16);
        printf("NwkSKey: 0x");
        hexdump(nodebase_node_pragma[loop].NwkSKey,16);
        node_cnt ++;
    }
    else
    {
        return -1;
    }
    return loop;
}

void *get_msg_to_send(uint32_t DevAddr)
{
    return NULL;
}
