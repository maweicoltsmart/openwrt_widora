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
int16_t database_node_join(node_join_info_t* node,uint32_t jiffiesval)
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
                return -1;
            }else{
                nodebase_node_pragma[loop].state = enStateJoinning;
                nodebase_node_pragma[loop].snr = node->snr;
                nodebase_node_pragma[loop].chip = node->chip;
                nodebase_node_pragma[loop].rssi = node->rssi;
                nodebase_node_pragma[loop].jiffies1 = jiffiesval + LoRaMacParams.JoinAcceptDelay1;
                nodebase_node_pragma[loop].jiffies2 = jiffiesval + LoRaMacParams.JoinAcceptDelay2;
            }
            nodebase_node_pragma[loop].timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
            if(nodebase_node_pragma[loop].timer)
            {
                init_timer(nodebase_node_pragma[loop].timer);
                nodebase_node_pragma[loop].timer->function = get_msg_to_send;
                nodebase_node_pragma[loop].timer->data = (unsigned long)loop;
                nodebase_node_pragma[loop].timer->expires = nodebase_node_pragma[loop].jiffies1;
                add_timer(nodebase_node_pragma[loop].timer);
            }
            /*list_for_each_entry(pos, &nodebase_node_pragma[loop].lora_tx_list.list,list){
                list_del(&pos->list);
                kfree(pos->buffer);
                kfree(pos);
            }*/
            //INIT_LIST_HEAD(&nodebase_node_pragma[loop].lora_tx_list.list);
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
        nodebase_node_pragma[loop].jiffies1 = jiffiesval + LoRaMacParams.JoinAcceptDelay1;
        nodebase_node_pragma[loop].jiffies2 = jiffiesval + LoRaMacParams.JoinAcceptDelay2;
        nodebase_node_pragma[loop].timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
        if(nodebase_node_pragma[loop].timer)
        {
            init_timer(nodebase_node_pragma[loop].timer);
            nodebase_node_pragma[loop].timer->function = get_msg_to_send;
            nodebase_node_pragma[loop].timer->data = (unsigned long)loop;
            nodebase_node_pragma[loop].timer->expires = nodebase_node_pragma[loop].jiffies1;
            add_timer(nodebase_node_pragma[loop].timer);
        }
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

uint8_t verify_net_addr(uint32_t addr)
{
    return(addr < node_cnt)?true:false;
}
void get_msg_to_send( unsigned long index )
{
    struct list_head *pos;
    struct lora_tx_data *get;
    unsigned char radiotxbuffer[2][300];
    uint8_t *LoRaMacBuffer = radiotxbuffer[0];
    uint8_t *pkg;
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t *fCtrl;
    uint16_t i;
    uint8_t pktHeaderLen = 0;
    uint32_t mic = 0;
    void* payload;

    uint16_t LoRaMacBufferPktLen = 0;

    switch(nodebase_node_pragma[index].state)
    {
        case enStateInit:
            break;
        case enStateJoinning:
            //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
            //p2 = (struct lora_tx_data *)&radiotxbuffer[1];
            pkg = (uint8_t*)&radiotxbuffer[1][0];
            macHdr.Bits.MType = FRAME_TYPE_JOIN_ACCEPT;
            pkg[0] = macHdr.Value;
            memcpy(&pkg[1],gateway_pragma.AppNonce,3); // AppNonce
            memcpy(&pkg[1 + 3],gateway_pragma.NetID,3); // NetID
            memcpy(&pkg[1 + 3 + 3],nodebase_node_pragma[index].DevAddr,4);  // DevAddr
            pkg[1 + 3 + 3 + 4] = 0; //DLSettings
            pkg[1 + 3 + 3 + 4 + 1] = 0; //RxDelay
            LoRaMacJoinComputeMic(radiotxbuffer[1],13,gateway_pragma.APPKEY,(uint32_t*)&pkg[1 + 3 + 3 + 4 + 1 + 1]);   // mic
            debug_output_data(pkg,17);
            LoRaMacJoinDecrypt(&pkg[1],12 + 4,gateway_pragma.APPKEY,&radiotxbuffer[0][1]);
            pkg = (uint8_t*)&radiotxbuffer[0][0];
            pkg[0]= macHdr.Value;
            //p2 = (struct lora_tx_data *)&radiotxbuffer[0];
            //p2->chip = p1->chip;
            //p2->size = 17;
            //update_node_info(index,p1->chip);
            //RadioTxMsgListAdd(index,pkg,17);
            nodebase_node_pragma[index].state = enStateJoined;
            //p2->freq = CN470_FIRST_RX1_CHANNEL + ( p2->chip ) * CN470_STEPWIDTH_RX1_CHANNEL;
            //p2->freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
            //Radio.Send(radiotxbuffer[1],17);
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
            DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_TX_START);
            DEBUG_OUTPUT_DATA(get->buffer,get->size);
            Radio.Send(nodebase_node_pragma[index].chip,pkg,17);
            debug_output_data(pkg,17);
            break;
        case enStateJoined:
            if(nodebase_node_pragma[index].repeatlen > 0)
            {
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
                DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_TX_START);
                DEBUG_OUTPUT_DATA(get->buffer,get->size);
                Radio.Send(nodebase_node_pragma[index].chip,nodebase_node_pragma[index].repeatbuf,nodebase_node_pragma[index].repeatlen);
                debug_output_data(nodebase_node_pragma[index].repeatbuf,nodebase_node_pragma[index].repeatlen);
                printk("send again\r\n");
                if(time_after(jiffies,(unsigned long)nodebase_node_pragma[index].jiffies2))
                {
                    printk("start timer RX2\r\n");
                    init_timer(nodebase_node_pragma[index].timer);
                    nodebase_node_pragma[index].timer->function = get_msg_to_send;
                    nodebase_node_pragma[index].timer->data = (unsigned long)index;
                    nodebase_node_pragma[index].timer->expires = nodebase_node_pragma[index].jiffies2;
                    add_timer(nodebase_node_pragma[index].timer);
                }
                else
                {
                    printk("delete timer\r\n");
                    del_timer(nodebase_node_pragma[index].timer);
                    kfree(nodebase_node_pragma[index].timer);
                    nodebase_node_pragma[index].timer = NULL;
                }
                break;
            }
            pos = nodebase_node_pragma[index].lora_tx_list.list.next;
            get = list_entry(pos, struct lora_tx_data, list);
            if((!get) && (!nodebase_node_pragma[index].is_ack_req) && (!(nodebase_node_pragma[index].cmdlen > 0)))
            {
                del_timer(nodebase_node_pragma[index].timer);
                kfree(nodebase_node_pragma[index].timer);
                nodebase_node_pragma[index].timer = NULL;
                break;
            }
            payload = get->buffer;
            macHdr.Bits.MType = FRAME_TYPE_DATA_CONFIRMED_DOWN;
            if(nodebase_node_pragma[index].is_ack_req)
            {
                fCtrl->Bits.Ack = 1;
            }
            // device addres
            LoRaMacBuffer[pktHeaderLen++] = ( index ) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = ( index >> 8 ) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = ( index >> 16 ) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = ( index >> 24 ) & 0xFF;

            LoRaMacBuffer[pktHeaderLen++] = fCtrl->Value;

            LoRaMacBuffer[pktHeaderLen++] = nodebase_node_pragma[index].sequenceCounter_Down & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = ( nodebase_node_pragma[index].sequenceCounter_Down >> 8 ) & 0xFF;

            // Copy the MAC commands which must be re-send into the MAC command buffer
            //memcpy1( &MacCommandsBuffer[MacCommandsBufferIndex], MacCommandsBufferToRepeat, MacCommandsBufferToRepeatIndex );
            //MacCommandsBufferIndex += MacCommandsBufferToRepeatIndex;

            if( ( get != NULL ))// && ( LoRaMacTxPayloadLen > 0 ) )
            {
                    if( nodebase_node_pragma[index].cmdlen > 0 )
                    {
                        fCtrl->Bits.FOptsLen = nodebase_node_pragma[index].cmdlen;

                        // Update FCtrl field with new value of OptionsLength
                        LoRaMacBuffer[0x05] = fCtrl->Value;
                        for( i = 0; i < nodebase_node_pragma[index].cmdlen; i++ )
                        {
                            LoRaMacBuffer[pktHeaderLen++] = nodebase_node_pragma[index].cmdbuf[i];
                        }
                    }
                    else
                    {
                        /*LoRaMacTxPayloadLen = MacCommandsBufferIndex;
                        payload = MacCommandsBuffer;
                        framePort = 0;*/
                    }
            }
            else
            {
                /*if( ( MacCommandsBufferIndex > 0 ) && ( MacCommandsInNextTx == true ) )
                {
                    LoRaMacTxPayloadLen = MacCommandsBufferIndex;
                    payload = MacCommandsBuffer;
                    framePort = 0;
                }*/
            }
            /*MacCommandsInNextTx = false;
            // Store MAC commands which must be re-send in case the device does not receive a downlink anymore
            MacCommandsBufferToRepeatIndex = ParseMacCommandsToRepeat( MacCommandsBuffer, MacCommandsBufferIndex, MacCommandsBufferToRepeat );
            if( MacCommandsBufferToRepeatIndex > 0 )
            {
                MacCommandsInNextTx = true;
            }
*/
            if( ( payload != NULL ))// && ( LoRaMacTxPayloadLen > 0 ) )
            {
                LoRaMacBuffer[pktHeaderLen++] = get->fPort;

                if( get->fPort == 0 )
                {
                    // Reset buffer index as the mac commands are being sent on port 0
                    nodebase_node_pragma[index].cmdlen = 0;
                    LoRaMacPayloadEncrypt( (uint8_t* ) payload, nodebase_node_pragma[index].cmdlen, nodebase_node_pragma[index].NwkSKey, index, DOWN_LINK, nodebase_node_pragma[index].sequenceCounter_Down, &LoRaMacBuffer[pktHeaderLen] );
                }
                else
                {
                    LoRaMacPayloadEncrypt( (uint8_t* ) payload, get->size, nodebase_node_pragma[index].AppSKey, index, DOWN_LINK, nodebase_node_pragma[index].sequenceCounter_Down, &LoRaMacBuffer[pktHeaderLen] );
                }
            }
            LoRaMacBufferPktLen = pktHeaderLen + get->size;

            LoRaMacComputeMic( LoRaMacBuffer, LoRaMacBufferPktLen, nodebase_node_pragma[index].NwkSKey, index, DOWN_LINK, nodebase_node_pragma[index].sequenceCounter_Down, &mic );

            LoRaMacBuffer[LoRaMacBufferPktLen + 0] = mic & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 1] = ( mic >> 8 ) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 2] = ( mic >> 16 ) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 3] = ( mic >> 24 ) & 0xFF;

            LoRaMacBufferPktLen += LORAMAC_MFR_LEN;
            nodebase_node_pragma[index].sequenceCounter_Down ++;
            nodebase_node_pragma[index].repeatbuf = kmalloc(LoRaMacBufferPktLen,GFP_KERNEL);
            memcpy(nodebase_node_pragma[index].repeatbuf,LoRaMacBuffer,LoRaMacBufferPktLen);
            nodebase_node_pragma[index].repeatlen = LoRaMacBufferPktLen;

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
            DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_TX_START);
            DEBUG_OUTPUT_DATA(get->buffer,get->size);
            Radio.Send(nodebase_node_pragma[index].chip,LoRaMacBuffer,LoRaMacBufferPktLen);
            debug_output_data(LoRaMacBuffer,LoRaMacBufferPktLen);

            list_del(&get->list);
            if(get->size > 0)
            {
                kfree(get->buffer);
            }
            kfree(get);

            if(time_after(jiffies,(unsigned long)nodebase_node_pragma[index].jiffies2))
            {
                printk("start timer RX2\r\n");
                init_timer(nodebase_node_pragma[index].timer);
                nodebase_node_pragma[index].timer->function = get_msg_to_send;
                nodebase_node_pragma[index].timer->data = (unsigned long)index;
                nodebase_node_pragma[index].timer->expires = nodebase_node_pragma[index].jiffies2;
                add_timer(nodebase_node_pragma[index].timer);
            }
            else
            {
                printk("delete timer\r\n");
                del_timer(nodebase_node_pragma[index].timer);
                kfree(nodebase_node_pragma[index].timer);
                nodebase_node_pragma[index].timer = NULL;
            }
            break;
        default:
            //return LORAMAC_STATUS_SERVICE_UNKNOWN;
            break;
    }
    /*pos = nodebase_node_pragma[index].lora_tx_list.list.next;
    get = list_entry(pos, struct lora_tx_data, list);
    if(!get)
    {
        del_timer(nodebase_node_pragma[index].timer);
        kfree(nodebase_node_pragma[index].timer);
        nodebase_node_pragma[index].timer = NULL;
        return;
    }
*/
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
    DEBUG_OUTPUT_EVENT(nodebase_node_pragma[index].chip,EV_TX_START);
    DEBUG_OUTPUT_DATA(get->buffer,get->size);
    Radio.Send(nodebase_node_pragma[index].chip,get->buffer,get->size);
    debug_output_data(get->buffer,get->size);
    if(time_after(jiffies,(unsigned long)nodebase_node_pragma[index].jiffies2))
    {
        init_timer(nodebase_node_pragma[index].timer);
        nodebase_node_pragma[index].timer->function = get_msg_to_send;
        nodebase_node_pragma[index].timer->data = (unsigned long)index;
        nodebase_node_pragma[index].timer->expires = nodebase_node_pragma[index].jiffies2;
        add_timer(nodebase_node_pragma[index].timer);
    }
    else
    {
        del_timer(nodebase_node_pragma[index].timer);
        kfree(nodebase_node_pragma[index].timer);
        nodebase_node_pragma[index].timer = NULL;
    }

    list_del(&get->list);
    if(get->size > 0)
    {
        kfree(get->buffer);
    }
    kfree(get);
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

void update_node_info(int index,int chip)
{
    nodebase_node_pragma[index].chip = chip;
}


