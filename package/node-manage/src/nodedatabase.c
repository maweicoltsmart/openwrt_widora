#include "nodedatabase.h"

#define LORAWAN_APPLICATION_KEY                     { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }

gateway_pragma_t gateway_pragma = {
    .APPKEY = LORAWAN_APPLICATION_KEY,
};
node_pragma_t nodebase_node_pragma[MAX_NODE];
static int16_t node_cnt = 0;

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
            hexdump((const unsigned char *)node->APPEUI,8);
            printf("DevEUI: 0x");
            hexdump((const unsigned char *)node->DevEUI,8);
            printf("DevNonce: 0x");
            hexdump((const unsigned char *)&node->DevNonce,2);
            printf("APPKEY: 0x");
            hexdump((const unsigned char *)nodebase_node_pragma[loop].AppSKey,16);
            printf("NwkSKey: 0x");
            hexdump((const unsigned char *)nodebase_node_pragma[loop].NwkSKey,16);
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
        hexdump((const unsigned char *)node->APPEUI,8);
        printf("DevEUI: 0x");
        hexdump((const unsigned char *)node->DevEUI,8);
        printf("DevNonce: 0x");
        hexdump((const unsigned char *)&node->DevNonce,2);
        printf("APPKEY: 0x");
        hexdump((const unsigned char *)nodebase_node_pragma[loop].AppSKey,16);
        printf("NwkSKey: 0x");
        hexdump((const unsigned char *)nodebase_node_pragma[loop].NwkSKey,16);
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
