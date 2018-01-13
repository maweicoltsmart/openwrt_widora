#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/time.h>

#include "nodedatabase.h"
#include "LoRaMacCrypto.h"
#include "radio.h"
#include "LoRaMac.h"
#include "proc.h"
#include "debug.h"

#define LORAWAN_APPLICATION_KEY                     { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }

const gateway_pragma_t gateway_pragma = {
    .APPKEY = LORAWAN_APPLICATION_KEY,
    .AppNonce = {0x12,0x34,0x56},
    .NetID  ={0x78,0x9a,0xbc},
};
node_pragma_t nodebase_node_pragma[MAX_NODE];
static int16_t node_cnt = 0;

void database_init(void)
{
    int loop = 0;
    memset(nodebase_node_pragma,0,sizeof(node_pragma_t) * MAX_NODE);
    for(loop = 0;loop < MAX_NODE;loop ++)
    {
        INIT_LIST_HEAD(&nodebase_node_pragma[loop].lora_tx_list.list);
        nodebase_node_pragma[loop].state = enStateInit;
    }
}
int16_t node_database_join(node_join_info_t* node)
{
    uint16_t loop;
    uint8_t tmp[6];
    //time_t   now;
    //struct timex  txc;
    //struct rtc_time tm;
    //struct lora_tx_data *pos;

    for(loop = 0;loop < node_cnt;loop ++)
    {
        if(!memcmp(node->DevEUI,nodebase_node_pragma[loop].DevEUI,8)){
            memcpy(tmp,gateway_pragma.AppNonce,3);
            memcpy(&tmp[3],gateway_pragma.NetID,3);
            LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, tmp, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
            nodebase_node_pragma[loop].DevNonce = node->DevNonce;
            //time(&now);
            /* 获取当前的UTC时间 */
            do_gettimeofday(&(nodebase_node_pragma[loop].LastComunication.time));

            *(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
            node_cnt ++;
            DEBUG_OUTPUT_INFO("APPEUI: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)node->APPEUI,8);
            DEBUG_OUTPUT_INFO("DevEUI: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)node->DevEUI,8);
            DEBUG_OUTPUT_INFO("DevNonce: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)&node->DevNonce,2);
            DEBUG_OUTPUT_INFO("APPKEY: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)nodebase_node_pragma[loop].AppSKey,16);
            DEBUG_OUTPUT_INFO("NwkSKey: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)nodebase_node_pragma[loop].NwkSKey,16);
            if(nodebase_node_pragma[loop].state == enStateJoinning)
            {
                printk("%s, %d\r\n",__func__,__LINE__);
                return -1;
            }else{
                nodebase_node_pragma[loop].state = enStateJoinning;
                nodebase_node_pragma[loop].snr = node->snr;
                nodebase_node_pragma[loop].chip = node->chip;
                nodebase_node_pragma[loop].rssi = node->rssi;
                nodebase_node_pragma[loop].jiffies = node->jiffies;
                nodebase_node_pragma[loop].state = enStateJoinning;
                node_prepare_joinaccept_package(loop);
                node_timer_start(loop);
            }

            return loop;
        }
    }
    if(loop < MAX_NODE){
        nodebase_node_pragma[loop].DevNonce = node->DevNonce;
        memcpy(nodebase_node_pragma[loop].APPEUI, node->APPEUI, 8);
        memcpy(nodebase_node_pragma[loop].DevEUI, node->DevEUI, 8);
        //time(&now);
        //time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
        //nodebase_node_pragma[loop].LastComunication   =   localtime(&now);
        do_gettimeofday(&(nodebase_node_pragma[loop].LastComunication.time));
        memcpy(tmp,gateway_pragma.AppNonce,3);
        memcpy(&tmp[3],gateway_pragma.NetID,3);
        LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, tmp, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
        *(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
        DEBUG_OUTPUT_INFO("APPEUI: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)node->APPEUI,8);
        DEBUG_OUTPUT_INFO("DevEUI: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)node->DevEUI,8);
        DEBUG_OUTPUT_INFO("DevNonce: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)&node->DevNonce,2);
        DEBUG_OUTPUT_INFO("APPKEY: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)nodebase_node_pragma[loop].AppSKey,16);
        DEBUG_OUTPUT_INFO("NwkSKey: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)nodebase_node_pragma[loop].NwkSKey,16);
        nodebase_node_pragma[loop].state = enStateJoinning;
        nodebase_node_pragma[loop].snr = node->snr;
        nodebase_node_pragma[loop].chip = node->chip;
        nodebase_node_pragma[loop].rssi = node->rssi;
        nodebase_node_pragma[loop].jiffies = node->jiffies;
        nodebase_node_pragma[loop].state = enStateJoinning;
        node_prepare_joinaccept_package(loop);
        node_timer_start(loop);
        node_cnt ++;
    }
    else
    {
        return -1;
    }
    /*list_for_each_entry(pos, &nodebase_node_pragma[loop].lora_tx_list.list,list){
                list_del(&pos->list);
                kfree(pos->buffer);
                kfree(pos);
            }*/
    //INIT_LIST_HEAD(&nodebase_node_pragma[loop].lora_tx_list.list);
    return loop;
}

uint8_t node_verify_net_addr(uint32_t addr)
{
    return(addr < node_cnt)?true:false;
}

void node_delete_repeat_buf(uint32_t index)
{
    nodebase_node_pragma[index].repeatlen = 0;
}

void node_timer_stop(uint32_t index)
{
    if(nodebase_node_pragma[index].timer1 != NULL)
    {
        del_timer(nodebase_node_pragma[index].timer1);
    }
    if(nodebase_node_pragma[index].timer2 != NULL)
    {
        del_timer(nodebase_node_pragma[index].timer2);
    }
}

void node_timer_start(uint32_t index)
{
    node_timer_stop(index);

    if(nodebase_node_pragma[index].timer1 == NULL)
    {
        nodebase_node_pragma[index].timer1 = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
        init_timer(nodebase_node_pragma[index].timer1);
        nodebase_node_pragma[index].timer1->function = node_get_msg_to_send;
        nodebase_node_pragma[index].timer1->data = (unsigned long)index;
    }
    if(nodebase_node_pragma[index].timer2 == NULL)
    {
        nodebase_node_pragma[index].timer2 = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
        init_timer(nodebase_node_pragma[index].timer2);
        nodebase_node_pragma[index].timer2->function = node_get_msg_to_send;
        nodebase_node_pragma[index].timer2->data = (unsigned long)index;
    }
    if(nodebase_node_pragma[index].timer1)
    {
        nodebase_node_pragma[index].timer1->expires = nodebase_node_pragma[index].jiffies1;
        add_timer(nodebase_node_pragma[index].timer1);
    }
    if(nodebase_node_pragma[index].timer2)
    {
        nodebase_node_pragma[index].timer2->expires = nodebase_node_pragma[index].jiffies2;
        add_timer(nodebase_node_pragma[index].timer2);
    }
}

int node_get_data_to_send(uint32_t index,uint8_t *buffer,uint8_t *more,uint8_t *fPort)
{
    struct list_head *pos;
    struct lora_tx_data *get;
    int len;
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    if(list_empty(&nodebase_node_pragma[index].lora_tx_list.list))
    {
        *more = false;
        return -1;
    }
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    pos = nodebase_node_pragma[index].lora_tx_list.list.next;
    get = list_entry(pos, struct lora_tx_data, list);
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    memcpy(buffer,get->buffer,get->size);
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    len = get->size;
    if(list_empty(&nodebase_node_pragma[index].lora_tx_list.list))
    {
        //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
        *more = false;
    }
    else
    {
        *more = true;
    }
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    return len;
}

void node_prepare_joinaccept_package( unsigned long index )
{
    //unsigned char LoRaMacBufferFinal[256];
    uint8_t payload[256];
    uint8_t *LoRaMacBuffer;
    //uint8_t *pkg;
    LoRaMacHeader_t macHdr;
    //LoRaMacFrameCtrl_t fCtrl;
    //uint16_t i;
    //uint8_t pktHeaderLen = 0;
    //uint32_t mic = 0;
    //void* payload;
    //uint8_t more;
    //uint16_t LoRaMacBufferPktLen = 0;

    nodebase_node_pragma[index].jiffies1 = nodebase_node_pragma[index].jiffies + LoRaMacParams.JoinAcceptDelay1;
    nodebase_node_pragma[index].jiffies2 = nodebase_node_pragma[index].jiffies + LoRaMacParams.JoinAcceptDelay2;
    LoRaMacBuffer = nodebase_node_pragma[index].repeatbuf;
    macHdr.Bits.MType = FRAME_TYPE_JOIN_ACCEPT;
    payload[0] = macHdr.Value;
    LoRaMacBuffer[0] = macHdr.Value;
    memcpy(&payload[1],gateway_pragma.AppNonce,3); // AppNonce
    memcpy(&payload[1 + 3],gateway_pragma.NetID,3); // NetID
    memcpy(&payload[1 + 3 + 3],nodebase_node_pragma[index].DevAddr,4);  // DevAddr
    payload[1 + 3 + 3 + 4] = 0; //DLSettings
    payload[1 + 3 + 3 + 4 + 1] = 0; //RxDelay
    LoRaMacJoinComputeMic(payload,13,gateway_pragma.APPKEY,(uint32_t*)&payload[1 + 3 + 3 + 4 + 1 + 1]);   // mic
    LoRaMacJoinDecrypt(&payload[1],12 + 4,gateway_pragma.APPKEY,&LoRaMacBuffer[1]);

    nodebase_node_pragma[index].repeatlen = 17;
    nodebase_node_pragma[index].state ++;
}

void node_prepare_data_package( unsigned long index )
{
    //unsigned char LoRaMacBufferFinal[256];
    uint8_t payload[256];
    uint8_t *LoRaMacBuffer;
    //uint8_t *pkg;
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t fCtrl;
    //uint16_t i;
    uint8_t pktHeaderLen = 0;
    uint32_t mic = 0;
    //void* payload;
    uint8_t more;
    uint16_t LoRaMacBufferPktLen = 0;
    int len;
    uint8_t fPort;
    bool newpkg = false;
    if(nodebase_node_pragma[index].state != enStateJoined)
    {
        return;
    }

    LoRaMacBuffer = nodebase_node_pragma[index].repeatbuf;
    len = node_get_data_to_send(index,payload,&more,&fPort);
    if((len > 0) || ( nodebase_node_pragma[index].cmdlen > 0 ))
    {
        macHdr.Bits.MType = FRAME_TYPE_DATA_CONFIRMED_DOWN;
        nodebase_node_pragma[index].need_ack = true;
        newpkg = true;
    }
    else
    {
        macHdr.Bits.MType = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
        nodebase_node_pragma[index].need_ack = false;
    }
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    fCtrl.Bits.FPending = more;

    if(nodebase_node_pragma[index].is_ack_req)
    {
        fCtrl.Bits.Ack = 1;
        newpkg = true;
    }
    LoRaMacBuffer[pktHeaderLen++] = macHdr.Value;

    LoRaMacBuffer[pktHeaderLen++] = ( index ) & 0xFF;
    LoRaMacBuffer[pktHeaderLen++] = ( index >> 8 ) & 0xFF;
    LoRaMacBuffer[pktHeaderLen++] = ( index >> 16 ) & 0xFF;
    LoRaMacBuffer[pktHeaderLen++] = ( index >> 24 ) & 0xFF;

    LoRaMacBuffer[pktHeaderLen++] = fCtrl.Value;

    LoRaMacBuffer[pktHeaderLen++] = nodebase_node_pragma[index].sequenceCounter_Down & 0xFF;
    LoRaMacBuffer[pktHeaderLen++] = ( nodebase_node_pragma[index].sequenceCounter_Down >> 8 ) & 0xFF;

    // Copy the MAC commands which must be re-send into the MAC command buffer
    //memcpy1( &MacCommandsBuffer[MacCommandsBufferIndex], MacCommandsBufferToRepeat, MacCommandsBufferToRepeatIndex );
    //MacCommandsBufferIndex += MacCommandsBufferToRepeatIndex;
    nodebase_node_pragma[index].cmdlen = 4;
    nodebase_node_pragma[index].cmdbuf[0] = stRadioCfg_Tx.datarate[nodebase_node_pragma[index].chip] << 4;
    nodebase_node_pragma[index].cmdbuf[0] |= stRadioCfg_Tx.power & 0x0f;//stRadioCfg_Tx.datarate[nodebase_node_pragma[index].chip] & 0x0f;
    *(uint16_t*)&nodebase_node_pragma[index].cmdbuf[1] = (uint16_t)1 << stRadioCfg_Rx.freq_rx[stRadioCfg_Rx.channel[nodebase_node_pragma[index].chip]];
    nodebase_node_pragma[index].cmdbuf[3] = 0x00;
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);

    #if 0
    if( nodebase_node_pragma[index].cmdlen > 0 )
    {
        fCtrl.Bits.FOptsLen = nodebase_node_pragma[index].cmdlen;

        // Update FCtrl field with new value of OptionsLength
        LoRaMacBuffer[0x05] = fCtrl.Value;
        for( i = 0; i < nodebase_node_pragma[index].cmdlen; i++ )
        {
            LoRaMacBuffer[pktHeaderLen++] = nodebase_node_pragma[index].cmdbuf[i];
        }
        need_tx = true;
    }
    else
    {
        fCtrl.Bits.FOptsLen = 0;
        LoRaMacBuffer[0x05] = fCtrl.Value;
        /*LoRaMacTxPayloadLen = MacCommandsBufferIndex;
        payload = MacCommandsBuffer;
        framePort = 0;*/
    }
    #endif
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    if( len > 0 )// && ( LoRaMacTxPayloadLen > 0 ) )
    {
        LoRaMacBuffer[pktHeaderLen++] = fPort;
        LoRaMacBufferPktLen = pktHeaderLen + len;
        LoRaMacPayloadEncrypt( (uint8_t* ) payload, len, nodebase_node_pragma[index].AppSKey, index, DOWN_LINK, nodebase_node_pragma[index].sequenceCounter_Down, &LoRaMacBuffer[pktHeaderLen] );
    }
    else
    {
        LoRaMacBuffer[pktHeaderLen++] = fPort;
        LoRaMacBufferPktLen = pktHeaderLen;
    }
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    LoRaMacComputeMic( LoRaMacBuffer, LoRaMacBufferPktLen, nodebase_node_pragma[index].NwkSKey, index, DOWN_LINK, nodebase_node_pragma[index].sequenceCounter_Down, &mic );

    LoRaMacBuffer[LoRaMacBufferPktLen + 0] = mic & 0xFF;
    LoRaMacBuffer[LoRaMacBufferPktLen + 1] = ( mic >> 8 ) & 0xFF;
    LoRaMacBuffer[LoRaMacBufferPktLen + 2] = ( mic >> 16 ) & 0xFF;
    LoRaMacBuffer[LoRaMacBufferPktLen + 3] = ( mic >> 24 ) & 0xFF;

    LoRaMacBufferPktLen += LORAMAC_MFR_LEN;
    nodebase_node_pragma[index].sequenceCounter_Down ++;
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    memcpy(nodebase_node_pragma[index].repeatbuf,LoRaMacBuffer,LoRaMacBufferPktLen);
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    nodebase_node_pragma[index].repeatlen = LoRaMacBufferPktLen;

    if(newpkg == true)
    {
        nodebase_node_pragma[index].state = enStateRxWindow1;
        DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_DATA_PREPARE_TO_SEND);
        DEBUG_OUTPUT_DATA(nodebase_node_pragma[index].repeatbuf,nodebase_node_pragma[index].repeatlen);
        node_timer_start(index);
    }
    nodebase_node_pragma[index].state = enStateRxWindow1;
}

void node_get_msg_to_send( unsigned long index )
{
    if(nodebase_node_pragma[index].repeatlen > 0)
    {
        switch(nodebase_node_pragma[index].state)
        {
            case enStateJoinaccept1:
            case enStateRxWindow1:
                Radio.Sleep(nodebase_node_pragma[index].chip);
                Radio.SetTxConfig(nodebase_node_pragma[index].chip,
                stRadioCfg_Tx.modem,
                stRadioCfg_Tx.power,
                stRadioCfg_Tx.fdev,
                stRadioCfg_Tx.bandwidth,
                stRadioCfg_Tx.datarate[nodebase_node_pragma[index].chip],
                stRadioCfg_Tx.coderate,
                stRadioCfg_Tx.preambleLen,
                stRadioCfg_Tx.fixLen,
                stRadioCfg_Tx.crcOn,
                stRadioCfg_Tx.freqHopOn,
                stRadioCfg_Tx.hopPeriod,
                stRadioCfg_Tx.iqInverted,
                stRadioCfg_Tx.timeout);
                Radio.SetChannel(nodebase_node_pragma[index].chip,stRadioCfg_Tx.freq_tx[stRadioCfg_Tx.channel[nodebase_node_pragma[index].chip]]);
                Radio.Send(nodebase_node_pragma[index].chip,nodebase_node_pragma[index].repeatbuf,nodebase_node_pragma[index].repeatlen);
                node_delete_repeat_buf(index);
                break;
            case enStateJoinaccept2:
            case enStateRxWindow2:
                Radio.Sleep(nodebase_node_pragma[index].chip);
                Radio.SetTxConfig(nodebase_node_pragma[index].chip,
                stRadioCfg_Tx.modem,
                stRadioCfg_Tx.power,
                stRadioCfg_Tx.fdev,
                stRadioCfg_Tx.bandwidth,
                12,
                stRadioCfg_Tx.coderate,
                stRadioCfg_Tx.preambleLen,
                stRadioCfg_Tx.fixLen,
                stRadioCfg_Tx.crcOn,
                stRadioCfg_Tx.freqHopOn,
                stRadioCfg_Tx.hopPeriod,
                stRadioCfg_Tx.iqInverted,
                stRadioCfg_Tx.timeout);
                Radio.SetChannel(nodebase_node_pragma[index].chip,stRadioCfg_Tx.freq_tx[25]);
                Radio.Send(nodebase_node_pragma[index].chip,nodebase_node_pragma[index].repeatbuf,nodebase_node_pragma[index].repeatlen);
                node_delete_repeat_buf(index);
                break;
            default:
                break;
        }
        DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_TX_START);
    }
    else
    {
        DEBUG_OUTPUT_INFO("%d: %s",nodebase_node_pragma[index].chip,"NO_DATA_TO_SEND\r\n");
    }
    nodebase_node_pragma[index].state ++;
    if(nodebase_node_pragma[index].state > enStateRxWindow2)
    {
        nodebase_node_pragma[index].state = enStateJoined;
    }
}

void node_have_confirm(uint32_t addr)
{
    printk("%s, %d\r\n",__func__,__LINE__);
    node_delete_repeat_buf(addr);
    //node_timer_stop(addr);
}
void node_get_net_addr(uint32_t *addr,uint8_t *ieeeaddr)
{
    *addr = 0;
    return;
}

void node_get_ieeeaddr(uint32_t addr,uint8_t *ieeeaddr)
{
    memcpy(ieeeaddr,nodebase_node_pragma[addr].DevEUI,8);
    return;
}

int RadioTxMsgListAdd(struct lora_tx_data *p)
{
    //struct timer_list *timer;
    struct lora_tx_data *pst_lora_tx_list;
    uint8_t *buf = kmalloc(p->size,GFP_KERNEL);

    if(buf == NULL)
    {
        return -1;
    }
    /*timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
    if(timer == NULL)
    {
        kfree(buf);
        return -1;
    }*/
    pst_lora_tx_list = kmalloc(sizeof(struct lora_tx_data),GFP_KERNEL);
    if(pst_lora_tx_list == NULL)
    {
        kfree(buf);
        //kfree(timer);
        return -1;
    }
    pst_lora_tx_list->buffer = buf;
    //nodebase_node_pragma[index].timer = timer;
    memcpy(buf,p->buffer,p->size);
    memcpy(pst_lora_tx_list,p,sizeof(struct lora_tx_data));
    //pst_lora_tx_list->size = len;
    //nodebase_node_pragma[index].jiffies1 = jiffies + LoRaMacParams.JoinAcceptDelay1;
    //nodebase_node_pragma[index].jiffies2 = jiffies + LoRaMacParams.JoinAcceptDelay2;
    list_add_tail(&(pst_lora_tx_list->list), &nodebase_node_pragma[p->addres].lora_tx_list.list);
    /*init_timer(timer);
    timer->function = get_msg_to_send;
    timer->data = (unsigned long)index;
    timer->expires = nodebase_node_pragma[index].jiffies1;
    add_timer(timer);*/
    return 0;
}

void node_update_info(int index,struct lora_rx_data *p1)
{
    nodebase_node_pragma[index].chip = p1->chip;
    nodebase_node_pragma[index].jiffies = p1->jiffies;
    nodebase_node_pragma[index].snr = p1->snr;
    nodebase_node_pragma[index].rssi = p1->rssi;
    nodebase_node_pragma[index].jiffies1 = nodebase_node_pragma[index].jiffies + LoRaMacParams.ReceiveDelay1;
    nodebase_node_pragma[index].jiffies2 = nodebase_node_pragma[index].jiffies + LoRaMacParams.ReceiveDelay2;
}

/*void start_timer(uint32_t index,uint32_t jiffiesval)
{
    if(nodebase_node_pragma[index].timer != NULL)
    {
        return;
    }
    else
    {
        nodebase_node_pragma[index].timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
        if(nodebase_node_pragma[index].timer == NULL)
        {
            return;
        }
        init_timer(nodebase_node_pragma[index].timer);
        nodebase_node_pragma[index].timer->function = get_msg_to_send;
        nodebase_node_pragma[index].timer->data = (unsigned long)index;
        nodebase_node_pragma[index].jiffies1 = jiffiesval + LoRaMacParams.ReceiveDelay1;
        nodebase_node_pragma[index].jiffies2 = jiffiesval + LoRaMacParams.ReceiveDelay2;
        nodebase_node_pragma[index].timer->expires = nodebase_node_pragma[index].jiffies1;
        add_timer(nodebase_node_pragma[index].timer);
    }
}*/

