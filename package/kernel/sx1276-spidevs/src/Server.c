#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/slab.h>
#include "Server.h"
#include "debug.h"
#include "NodeDatabase.h"
#include "sx1276-cdev.h"
#include "LoRaMac.h"
#include "LoRaMacCrypto.h"

st_ServerMsgUpQueue stServerMsgUpQueueListHead;
st_ServerMsgDownQueue stServerMsgDownQueueListHead;

void ServerMsgInit(void){
	INIT_LIST_HEAD(&stServerMsgUpQueueListHead.list);
	INIT_LIST_HEAD(&stServerMsgDownQueueListHead.list);
}

void ServerMsgRemove(void){
	st_ServerMsgUpQueue *tmpup;
	struct list_head *posup, *qup;
	st_ServerMsgDownQueue *tmpdown;
	struct list_head *posdown, *qdown;
	
	list_for_each_safe(posup, qup, &stServerMsgUpQueueListHead.list){
		tmpup= list_entry(posup, st_ServerMsgUpQueue, list);
		if(tmpup->stServerMsgUp.enMsgUpFramType == en_MsgUpFramDataReceive)
		{
			kfree(tmpup->stServerMsgUp.Msg.stData2Server.payload);
		}
		list_del(posup);
		kfree(tmpup);
	}

	list_for_each_safe(posdown, qdown, &stServerMsgDownQueueListHead.list){
		tmpdown= list_entry(posdown, st_ServerMsgDownQueue, list);
		if(tmpdown->stServerMsgDown.enMsgUpFramType == en_MsgUpFramDataReceive)
		{
			kfree(tmpdown->stServerMsgDown.Msg.stData2Server.payload);
		}
		list_del(posdown);
		kfree(tmpdown);
	}
}

int ServerMsgUpListGet(const pst_ServerMsgUp pstServerMsgUp){
    struct list_head *pos;
    pst_ServerMsgUpQueue get;
	uint8_t *payload = NULL;
    if(list_empty(&stServerMsgUpQueueListHead.list))
    {
        return -1;
    }
    pos = stServerMsgUpQueueListHead.list.next;
    get = list_entry(pos, st_ServerMsgUpQueue, list);
	if(get->stServerMsgUp.enMsgUpFramType == en_MsgUpFramDataReceive)
	{
		payload = pstServerMsgUp->Msg.stData2Server.payload;
		memcpy(pstServerMsgUp,&get->stServerMsgUp,sizeof(st_ServerMsgUp));
		pstServerMsgUp->Msg.stData2Server.payload = payload;
		memcpy(pstServerMsgUp->Msg.stData2Server.payload,get->stServerMsgUp.Msg.stData2Server.payload,get->stServerMsgUp.Msg.stData2Server.size);
		kfree(get->stServerMsgUp.Msg.stData2Server.payload);
	}
	else
	{
		memcpy(pstServerMsgUp,&get->stServerMsgUp.Msg.stData2Server,sizeof(st_ServerMsgUp));
	}
    list_del(&get->list);
    kfree(get);
    return 0;
}
int ServerMsgUpListAdd(const pst_ServerMsgUp pstServerMsgUp){
	pst_ServerMsgUpQueue new;
    new = (pst_ServerMsgUpQueue)kmalloc(sizeof(st_ServerMsgUpQueue),GFP_KERNEL);
    if(!new)
    {
        return -1;
    }
	memcpy(&new->stServerMsgUp,pstServerMsgUp,sizeof(st_ServerMsgUp));
	if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramDataReceive)
	{
	    new->stServerMsgUp.Msg.stData2Server.payload = kmalloc(new->stServerMsgUp.Msg.stData2Server.size,GFP_KERNEL);
	    if(!(new->stServerMsgUp.Msg.stData2Server.payload))
	    {
	        kfree(new);
	        return -1;
	    }
		memcpy(new->stServerMsgUp.Msg.stData2Server.payload,pstServerMsgUp->Msg.stData2Server.payload,pstServerMsgUp->Msg.stData2Server.size);
	}
    list_add_tail(&(new->list), &stServerMsgUpQueueListHead.list);
	have_data = true;
	wake_up(&read_from_user_space_wait);
    return 0;
}
int ServerMsgDownListGet(pst_ServerMsgDown pstServerMsgDown){
	return 0;
}
void ServerMsgDownListAdd(pst_ServerMsgDown pstServerMsgDown){
	//uint32_t address;
	st_ServerMsgUp stServerMsgUp;
	LoRaMacHeader_t macHdr;
	LoRaMacFrameCtrl_t fCtrl;
	uint8_t LoRaMacBufferPktLen;
	uint32_t mic;
	if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramDataSend)
	{
		if(!NodeDatabaseVerifyNetAddr(pstServerMsgDown->Msg.stData2Node.DevAddr))
		{
			stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
			memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstServerMsgDown->Msg.stData2Node.DevAddr].stDevNetParameter.DevEUI,8);
			stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerOffline;
			ServerMsgUpListAdd(&stServerMsgUp);
			return;
		}
		if((stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf != NULL) && (stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len > 0))
		{
			stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
			memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstServerMsgDown->Msg.stData2Node.DevAddr].stDevNetParameter.DevEUI,8);
			stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerRadioBusy;
			ServerMsgUpListAdd(&stServerMsgUp);
			return;
		}
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len = pstServerMsgDown->Msg.stData2Node.size;
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf = (uint8_t *)kmalloc(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len,GFP_KERNEL);
		//memcpy(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf,pstServerMsgDown->Msg.stData2Node.payload,pstServerMsgDown->Msg.stData2Node.size);
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len = pstServerMsgDown->Msg.stData2Node.size;
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.fPort = pstServerMsgDown->Msg.stData2Node.fPort;
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].is_ack_req = pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest;
		//stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].have_ack = pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack;
		
		if(pstServerMsgDown->Msg.stData2Node.CtrlBits.AckRequest)
    	{
        	macHdr.Bits.MType = FRAME_TYPE_DATA_CONFIRMED_DOWN;
    	}

    	else
    	{
        	macHdr.Bits.MType = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    	}
		fCtrl.Value = 0;
		fCtrl.Bits.FPending = false;
		fCtrl.Bits.Ack = pstServerMsgDown->Msg.stData2Node.CtrlBits.Ack;
	
		stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[0] = macHdr.Value;
		*(uint32_t *)(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf + 1) = pstServerMsgDown->Msg.stData2Node.DevAddr;
		*(uint8_t *)(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf + 1 + 4) = fCtrl.Value;
		*(uint16_t *)(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf + 1 + 4 + 1) = stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].sequence_down;
		if(pstServerMsgDown->Msg.stData2Node.size > 0)
		{
			*(uint8_t *)(stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf + 1 + 4 + 1 + 2) = pstServerMsgDown->Msg.stData2Node.fPort;
			LoRaMacBufferPktLen = 1 + 4 + 1 + 2 + 1;
			LoRaMacPayloadEncrypt( pstServerMsgDown->Msg.stData2Node.payload, pstServerMsgDown->Msg.stData2Node.size, stNodeInfoToSave[pstServerMsgDown->Msg.stData2Node.DevAddr].stDevNetParameter.AppSKey, pstServerMsgDown->Msg.stData2Node.DevAddr, DOWN_LINK, stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].sequence_down,&stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[LoRaMacBufferPktLen]);
			LoRaMacBufferPktLen += pstServerMsgDown->Msg.stData2Node.size;
		}
		else
		{
			LoRaMacBufferPktLen = 1 + 4 + 1 + 2;
		}
		LoRaMacComputeMic( stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf, LoRaMacBufferPktLen, stNodeInfoToSave[pstServerMsgDown->Msg.stData2Node.DevAddr].stDevNetParameter.NwkSKey, pstServerMsgDown->Msg.stData2Node.DevAddr, DOWN_LINK, stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].sequence_down, &mic );
	    stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[LoRaMacBufferPktLen + 0] = mic & 0xFF;
	    stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[LoRaMacBufferPktLen + 1] = ( mic >> 8 ) & 0xFF;
	    stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[LoRaMacBufferPktLen + 2] = ( mic >> 16 ) & 0xFF;
	    stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.buf[LoRaMacBufferPktLen + 3] = ( mic >> 24 ) & 0xFF;
	    LoRaMacBufferPktLen += LORAMAC_MFR_LEN;
		stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len = LoRaMacBufferPktLen;
		stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].sequence_down ++;

		stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].state = enStateRxWindow1;
		printk("server down ,%d , %d\r\n",pstServerMsgDown->Msg.stData2Node.DevAddr,stNodeDatabase[pstServerMsgDown->Msg.stData2Node.DevAddr].stTxData.len);
	}
	else if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramConfirm)
	{
		if(!NodeDatabaseVerifyNetAddr(pstServerMsgDown->Msg.stConfirm2Node.DevAddr))
		{
			stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
			memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,stNodeInfoToSave[pstServerMsgDown->Msg.stConfirm2Node.DevAddr].stDevNetParameter.DevEUI,8);
			stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerOffline;
			ServerMsgUpListAdd(&stServerMsgUp);
			return;
		}
	}
	else
	{
		DEBUG_OUTPUT_INFO("error: unKnown Data Down Frame Type!\r\n");
	}
}

int ServerMsgDownProcess(void *data){	// Class C message to node
	pst_ServerMsgDown pstServerMsgDown = NULL;
	ServerMsgDownListGet(pstServerMsgDown);
	//	射频是否空闲
	//	节点剩余时间窗口是否足够发送此帧数据
	return 0;
}


