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
		payload = get->stServerMsgUp.Msg.stData2Server.payload;
		memcpy(pstServerMsgUp,&get->stServerMsgUp,sizeof(st_ServerMsgUp));
		get->stServerMsgUp.Msg.stData2Server.payload = payload;
		memcpy(pstServerMsgUp->Msg.stData2Server.payload,get->stServerMsgUp.Msg.stData2Server.payload,get->stServerMsgUp.Msg.stData2Server.size);
	}
	else
	{
		memcpy(pstServerMsgUp,&get->stServerMsgUp.Msg.stData2Server,sizeof(st_ServerMsgUp));
	}
    list_del(&get->list);
    kfree(get->stServerMsgUp.Msg.stData2Server.payload);
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
	uint32_t address;
	st_ServerMsgUp stServerMsgUp;
	if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramDataSend)
	{
		if(!NodeGetNetAddr(&address,pstServerMsgDown->Msg.stData2Node.DevEUI))
		{
			stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
			memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,pstServerMsgDown->Msg.stData2Node.DevEUI,8);
			stServerMsgUp.Msg.stConfirm2Server.enConfirm2Server = en_Confirm2ServerOffline;
			ServerMsgUpListAdd(&stServerMsgUp);
			return;
		}
	}
	else if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramConfirm)
	{
		if(!NodeGetNetAddr(&address,pstServerMsgDown->Msg.stData2Node.DevEUI))
		{
			stServerMsgUp.enMsgUpFramType = en_MsgUpFramConfirm;
			memcpy(stServerMsgUp.Msg.stConfirm2Server.DevEUI,pstServerMsgDown->Msg.stData2Node.DevEUI,8);
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


