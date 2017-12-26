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
	.NetID	={0x78,0x9a,0xbc},
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
	}
}
int16_t database_node_join(node_join_info_t* node)
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

void get_msg_to_send( unsigned long index )
{
    struct list_head *pos;
	struct lora_tx_data *get;
	pos = nodebase_node_pragma[index].lora_tx_list.list.next;
	get = list_entry(pos, struct lora_tx_data, list);
    if(!get)
    {
    	del_timer(nodebase_node_pragma[index].timer);
    	kfree(nodebase_node_pragma[index].timer);
		nodebase_node_pragma[index].timer = NULL;
        return;
    }
	
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

int RadioTxMsgListAdd(int index,uint8_t *data,int len)
{
	struct timer_list *timer;
	struct lora_tx_data *pst_lora_tx_list;
	uint8_t *buf = kmalloc(len,GFP_KERNEL);
	
	if(buf == NULL)
	{
		return -1;
	}
	timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
	if(timer == NULL)
	{
		kfree(buf);
		return -1;
	}
	pst_lora_tx_list = kmalloc(sizeof(struct lora_tx_data),GFP_KERNEL);
	if(pst_lora_tx_list == NULL)
	{
		kfree(buf);
		kfree(timer);
		return -1;
	}
	pst_lora_tx_list->buffer = buf;
	nodebase_node_pragma[index].timer = timer;
	memcpy(buf,data,len);
	pst_lora_tx_list->size = len;
	nodebase_node_pragma[index].jiffies1 = jiffies + LoRaMacParams.JoinAcceptDelay1;
	nodebase_node_pragma[index].jiffies2 = jiffies + LoRaMacParams.JoinAcceptDelay2;
	list_add_tail(&(pst_lora_tx_list->list), &nodebase_node_pragma[index].lora_tx_list.list);
	
	init_timer(timer);
	timer->function = get_msg_to_send;
    timer->data = (unsigned long)index;
    timer->expires = nodebase_node_pragma[index].jiffies1;
    add_timer(timer);
	return 0;
}

void update_node_info(int index,int chip)
{
	nodebase_node_pragma[index].chip = chip;
}


