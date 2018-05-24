#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include "NodeDatabase.h"
#include "debug.h"
#include "utilities.h"
#include "LoRaWAN.h"

uint32_t node_cnt = 0;
st_NodeInfoToSave stNodeInfoToSave[MAX_NODE];
st_NodeDatabase stNodeDatabase[MAX_NODE];

void NodeDatabaseReload(void);
void NodeDatabaseRestore(uint32_t addr);

void NodeDatabaseInit(void)
{
	uint32_t loop;
	NodeDatabaseReload();
	
    for(loop = 0;loop < MAX_NODE;loop ++)
    {
        stNodeDatabase[loop].state = enStateInit;
        stNodeDatabase[loop].jiffies = jiffies;
		init_timer(&stNodeDatabase[loop].timer1);
		init_timer(&stNodeDatabase[loop].timer2);
    }
}

void NodeDatabaseRemove(void)
{
	uint32_t loop;
	
    for(loop = 0;loop < MAX_NODE;loop ++)
    {
		del_timer(&stNodeDatabase[loop].timer1);
		del_timer(&stNodeDatabase[loop].timer2);
		stNodeDatabase[loop].stTxData.len = 0;
		//tasklet_kill(&stNodeDatabase[loop].tasklet);
		//if(loop < node_cnt)
		//NodeDatabaseRestore(loop);
    }
}

void NodeDatabaseReload(void)
{
	int len;
	struct file *fp;
    mm_segment_t fs;
    loff_t pos;
	uint16_t crc;
	
    DEBUG_OUTPUT_INFO("reload node database\r\n");
    fp =filp_open("/usr/nodedatabase",O_RDWR | O_CREAT,0644);
    if (IS_ERR(fp)){ 
        printk("%s ,%d error\r\n",__func__,__LINE__);
		goto resetdatabase;
    }
    fs =get_fs();
    set_fs(KERNEL_DS);
    pos =0; 
	len = vfs_read(fp,(char *)&crc, sizeof(crc), &pos);
	if(len < sizeof(crc))
	{
		printk("%s ,%d error\r\n",__func__,__LINE__);
		goto resetdatabase;
	}
	//printk("%s ,%d ,%04X\r\n",__func__,__LINE__,crc);
	pos += sizeof(crc);
	len = vfs_read(fp,(char *)&node_cnt, sizeof(node_cnt), &pos);
	if(len < sizeof(node_cnt))
	{
		printk("%s ,%d error\r\n",__func__,__LINE__);
		goto resetdatabase;
	}
	//printk("%s ,%d ,%08X\r\n",__func__,__LINE__,node_cnt);
	pos += sizeof(node_cnt);
    len = vfs_read(fp,(char *)stNodeInfoToSave, sizeof(st_NodeInfoToSave) * node_cnt, &pos);
	filp_close(fp,NULL);
    set_fs(fs);
    return;
    //if((len < sizeof(st_NodeInfoToSave) * node_cnt) || (crc != Crc16((uint8_t *)stNodeInfoToSave,sizeof(st_NodeInfoToSave) * node_cnt)))
    {
    	//printk("%s ,%d ,%d\r\n",__func__,len,sizeof(st_NodeInfoToSave) * node_cnt);
		//printk("%s ,%d ,%d\r\n",__func__,crc,Crc16((uint8_t *)stNodeInfoToSave,sizeof(st_NodeInfoToSave) * node_cnt));
resetdatabase:
    	node_cnt = 0;
    	memset(stNodeInfoToSave,0,sizeof(stNodeInfoToSave));
		printk("The node database verify error, all node must be rejoin again\r\n");
    }
}

void NodeDatabaseRestore(uint32_t addr)
{
	int len;
	struct file *fp;
    mm_segment_t fs;
    loff_t pos;
	uint16_t crc;

	DEBUG_OUTPUT_INFO("restore node database addr: %d\r\n",addr);
    fp =filp_open("/usr/nodedatabase",O_RDWR | O_CREAT,0644);
    if (IS_ERR(fp)){ 
        printk("%s ,%d error\r\n",__func__,__LINE__);
    }
    fs =get_fs();
    set_fs(KERNEL_DS);

    pos = 0;
	crc = 0;//Crc16((uint8_t *)stNodeInfoToSave,sizeof(st_NodeInfoToSave) * node_cnt);
    len = vfs_write(fp,(char *)&crc, sizeof(crc), &pos);
    if(len < sizeof(crc))
    {
    	printk("%s ,%d error\r\n",__func__,__LINE__);
    }
	//printk("%s ,%d ,%04X\r\n",__func__,__LINE__,crc);
    pos += sizeof(crc); 
    len = vfs_write(fp,(char *)&node_cnt, sizeof(node_cnt), &pos);
    if(len < sizeof(node_cnt))
    {
    	printk("%s ,%d error\r\n",__func__,__LINE__);
    }
	//printk("%s ,%d ,%08X\r\n",__func__,__LINE__,node_cnt);
    pos += sizeof(node_cnt); 
	pos += sizeof(st_NodeInfoToSave) * addr; 
    len = vfs_write(fp,(char *)&stNodeInfoToSave[addr], sizeof(st_NodeInfoToSave), &pos);
    if(len < sizeof(st_NodeInfoToSave))
    {
    	printk("%s ,%d error\r\n",__func__,__LINE__);
    }
    filp_close(fp,NULL);
    set_fs(fs);
}

uint32_t NodeDatabaseJoin(const pst_NodeJoinInfo node)
{
    uint32_t loop;
	
    for(loop = 0;loop < node_cnt;loop ++)
    {
        if(!memcmp(node->DevEUI,stNodeInfoToSave[loop].stDevNetParameter.DevEUI,8)){
            LoRaMacJoinComputeSKeys( stGatewayParameter.AppKey, stGatewayParameter.AppNonce, node->DevNonce, stNodeInfoToSave[loop].stDevNetParameter.NwkSKey, stNodeInfoToSave[loop].stDevNetParameter.AppSKey );
            stNodeInfoToSave[loop].stDevNetParameter.DevNonce = node->DevNonce;
            stNodeInfoToSave[loop].stDevNetParameter.DevAddr = loop;
            DEBUG_OUTPUT_INFO("APPEUI: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)node->AppEUI,8);
            DEBUG_OUTPUT_INFO("DevEUI: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)node->DevEUI,8);
            DEBUG_OUTPUT_INFO("DevNonce: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)&node->DevNonce,2);
            DEBUG_OUTPUT_INFO("APPKEY: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)stNodeInfoToSave[loop].stDevNetParameter.AppSKey,16);
            DEBUG_OUTPUT_INFO("NwkSKey: 0x");
            DEBUG_OUTPUT_DATA((unsigned char *)stNodeInfoToSave[loop].stDevNetParameter.NwkSKey,16);
            /*if(stNodeDatabase[loop].state == enStateJoinning)
            {
                return -1;
            }else*/{
                stNodeDatabase[loop].snr = node->snr;
                stNodeDatabase[loop].chip = node->chip;
                stNodeDatabase[loop].rssi = node->rssi;
                stNodeDatabase[loop].jiffies = node->jiffies;
				stNodeInfoToSave[loop].classtype = node->classtype;
                stNodeDatabase[loop].state = enStateJoinning;
            }
			NodeDatabaseRestore(loop);

            return loop;
        }
    }
    if(loop < MAX_NODE){
        stNodeInfoToSave[loop].stDevNetParameter.DevNonce = node->DevNonce;
        memcpy(stNodeInfoToSave[loop].stDevNetParameter.AppEUI, node->AppEUI, 8);
        memcpy(stNodeInfoToSave[loop].stDevNetParameter.DevEUI, node->DevEUI, 8);
        LoRaMacJoinComputeSKeys( stGatewayParameter.AppKey, stGatewayParameter.AppNonce, node->DevNonce, stNodeInfoToSave[loop].stDevNetParameter.NwkSKey, stNodeInfoToSave[loop].stDevNetParameter.AppSKey );
        stNodeInfoToSave[loop].stDevNetParameter.DevAddr = loop;
        DEBUG_OUTPUT_INFO("APPEUI: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)node->AppEUI,8);
        DEBUG_OUTPUT_INFO("DevEUI: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)node->DevEUI,8);
        DEBUG_OUTPUT_INFO("DevNonce: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)&node->DevNonce,2);
        DEBUG_OUTPUT_INFO("APPKEY: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)stNodeInfoToSave[loop].stDevNetParameter.AppSKey,16);
        DEBUG_OUTPUT_INFO("NwkSKey: 0x");
        DEBUG_OUTPUT_DATA((unsigned char *)stNodeInfoToSave[loop].stDevNetParameter.NwkSKey,16);
        stNodeDatabase[loop].snr = node->snr;
        stNodeDatabase[loop].chip = node->chip;
        stNodeDatabase[loop].rssi = node->rssi;
        stNodeDatabase[loop].jiffies = node->jiffies;
		stNodeInfoToSave[loop].classtype = node->classtype;
        stNodeDatabase[loop].state = enStateJoinning;
        node_cnt ++;
    }
    else
    {
        return -1;
    }    
	NodeDatabaseRestore(loop);
    return loop;
}

void NodeDatabaseUpdateParameters(uint32_t addr, uint16_t fcnt, pst_RadioRxList pstRadioRxList)
{
	LoRaMacHeader_t macHdr;
	
	macHdr.Value = pstRadioRxList->stRadioRx.payload[0];
	stNodeDatabase[addr].snr = pstRadioRxList->stRadioRx.snr;
	stNodeDatabase[addr].chip = pstRadioRxList->stRadioRx.chip;
    stNodeDatabase[addr].rssi = pstRadioRxList->stRadioRx.rssi;
	stNodeDatabase[addr].jiffies = pstRadioRxList->jiffies;
	stNodeInfoToSave[addr].classtype = macHdr.Bits.RFU;
	stNodeDatabase[addr].stTxData.len = 0;
	if(fcnt == 0)
	{
		stNodeDatabase[addr].sequence_down = 0;
	}
    stNodeDatabase[addr].state = enStateJoined;
}
bool NodeDatabaseVerifyNetAddr(uint32_t addr)
{
    return(addr < node_cnt)?true:false;
}

bool NodeGetNetAddr(uint32_t *addr,uint8_t *deveui)
{
    uint32_t index = 0;
    for(index = 0;index < node_cnt;index ++)
    {
        if(memcmp(deveui,stNodeInfoToSave[index].stDevNetParameter.DevEUI,8) == 0)
        {
            *addr = index;
            return true;
        }
    }
    *addr = 0;
    return false;
}

bool NodeDatabaseGetDevEUI(uint32_t addr,uint8_t *deveui)
{
	if(NodeDatabaseVerifyNetAddr(addr))
	{
    	memcpy(deveui,stNodeInfoToSave[addr].stDevNetParameter.DevEUI,8);
    	return true;
	}
	else
	{
		return false;
	}
}

bool NodeDatabaseGetAppEUI(uint32_t addr,uint8_t *appeui)
{
	
	if(NodeDatabaseVerifyNetAddr(addr))
	{
    	memcpy(appeui,stNodeInfoToSave[addr].stDevNetParameter.AppEUI,8);
    	return true;
	}
	else
	{
		return false;
	}
}

