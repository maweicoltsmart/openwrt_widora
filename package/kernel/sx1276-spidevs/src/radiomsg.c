#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include "typedef.h"
#include "radiomsg.h"
#include "debug.h"
#include "nodedatabase.h"

extern bool have_data;
extern wait_queue_head_t read_from_user_space_wait;
static struct lora_rx_data lora_rx_list;
//static struct lora_tx_data lora_tx_list;
static struct lora_server_data_list lora_rx2server_list;

void RadioMsgListInit(void)
{
    INIT_LIST_HEAD(&lora_rx_list.list);
    INIT_LIST_HEAD(&lora_rx2server_list.list);
}
int RadioRxMsgListAdd( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    struct lora_rx_data *new;
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

int Radio2ServerMsgListAdd( uint32_t addr,uint8_t fPort,uint8_t *payload, uint16_t size )
{
    struct lora_server_data_list *new;
    lora_server_up_data_type *data;
    new = (struct lora_server_data_list *)kmalloc(sizeof(struct lora_server_data_list),GFP_KERNEL);
    if(!new)
    {
        return -1;
    }
    new->buffer = kmalloc(size + sizeof(lora_server_up_data_type),GFP_KERNEL);
    if(!(new->buffer))
    {
        kfree(new);
        return -1;
    }
    data = (lora_server_up_data_type *)new->buffer;
    node_get_ieeeaddr(addr,data->DevEUI);
    node_get_appeui(addr,data->APPEUI);
    data->DevAddr = addr;

    data->fPort = fPort;
    node_get_Battery(addr,&data->Battery);
    data->rssi = nodebase_node_pragma[addr].rssi;
    data->snr = nodebase_node_pragma[addr].snr;
    data->CtrlBits.AckRequest = nodebase_node_pragma[addr].is_ack_req;
    data->CtrlBits.Ack = nodebase_node_pragma[addr].have_ack;
    data->size = size;
    memcpy(new->buffer + sizeof(lora_server_up_data_type),payload,size);
    new->size = size + sizeof(lora_server_up_data_type);
    list_add_tail(&(new->list), &lora_rx2server_list.list);
    have_data = true;
    wake_up(&read_from_user_space_wait);
    //printk("wake_up\r\n");
    return 0;
}

int Radio2ServerMsgListGet( uint8_t *buffer )
{
    struct list_head *pos;
    struct lora_server_data_list *get;
    int len;
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    if(list_empty(&lora_rx2server_list.list))
    {
        return -1;
    }
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    pos = lora_rx2server_list.list.next;
    get = list_entry(pos, struct lora_server_data_list, list);
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    memcpy(buffer,get->buffer,get->size);
    //printk("%s, %d, %d\r\n",__func__,__LINE__,len);
    len = get->size;
    //printk("len = %d\r\n",len);
    list_del(&get->list);
    kfree(get->buffer);
    kfree(get);
    return len;
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
