#include <linux/timer.h>
#include <linux/slab.h>
#include "radiomsg.h"
#include "debug.h"

static struct lora_rx_data lora_rx_list;
//static struct lora_tx_data lora_tx_list;

void RadioMsgListInit(void)
{
	INIT_LIST_HEAD(&lora_rx_list.list);
    //INIT_LIST_HEAD(&lora_tx_list.list);
}
int RadioRxMsgListAdd( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	struct lora_rx_data *new;
	DEBUG_OUTPUT_DATA(payload,size);
	new = (struct lora_rx_data *)kmalloc(sizeof(struct lora_rx_data),GFP_KERNEL);
	if(!new)
	{
		return -1;
	}
	new->buffer = kmalloc(size,GFP_KERNEL);
    if(!(new->buffer))
    {
    	kfree(new);
        return -1;
    }
    memcpy(new->buffer,payload,size);
    new->jiffies = jiffies;
    new->chip = chip;
    new->size = size;
    new->rssi = rssi;
    new->snr = snr;
    list_add_tail(&(new->list), &lora_rx_list.list);
	return 0;
}

int RadioRxMsgListGet(uint8_t *buf)
{
	struct lora_rx_data *get;
    struct list_head *pos;

	if(list_empty(&lora_rx_list.list))
	{
		return -1;
	}
	else
	{
		pos = lora_rx_list.list.next;
    	get = list_entry(pos, struct lora_rx_data, list);
    	memcpy(buf,get,sizeof(struct lora_rx_data));
    	memcpy(&buf[sizeof(struct lora_rx_data)],get->buffer,get->size);
    	list_del(&get->list);
    	kfree(get->buffer);
    	kfree(get);
		return 0;
	}
}
