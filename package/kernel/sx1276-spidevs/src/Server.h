#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct{
	uint8_t ClassType;
    uint8_t DevEUI[8];
    uint32_t DevAddr;
    uint8_t AppEUI[8];
    uint8_t fPort;
    uint8_t Battery;
    int16_t rssi;
    int8_t snr;
    uint8_t size;
	uint8_t *payload;
    struct
    {
        uint8_t AckRequest      : 1;
		uint8_t Ack             : 1;
        uint8_t Reserve         : 6;
    }CtrlBits;
}st_Data2Server;

typedef enum{
	en_Confirm2ServerSuccess,	//	发送成功:确认帧收到确认或者；无需确认帧发送成功
	en_Confirm2ServerRadioBusy,	//	因为信道忙，发送超时
	en_Confirm2ServerOffline,	//	节点不在线
	en_Confirm2ServerTooLong,	//	包太大
	en_Confirm2ServerPortNotAllow,	// 端口号超出范围
	en_Confirm2ServerInLastDutyCycle,	// 发送缓冲区非空
	en_Confirm2ServerNodeNoAck,	// 节点未应答
	en_Confirm2ServerNodeNotOnRxWindow	// Class A节点不在接收窗口
}en_Confirm2Server,*pen_Confirm2Server;

typedef enum{
	en_MsgUpFramDataReceive,
	en_MsgUpFramConfirm
}en_MsgUpFramType,*pen_MsgUpFramType;
typedef struct{
	uint8_t ClassType;
    uint8_t DevEUI[8];
	uint32_t DevAddr;
    int16_t rssi;
    int8_t snr;
	en_Confirm2Server enConfirm2Server;
}st_Confirm2Server;

typedef struct{
	en_MsgUpFramType enMsgUpFramType;
	union{
		st_Data2Server stData2Server;
		st_Confirm2Server stConfirm2Server;
	}Msg;
}st_ServerMsgUp,*pst_ServerMsgUp;

typedef enum{
	en_MsgDownFramDataSend,
	en_MsgDownFramConfirm
}en_MsgDownFramType,*pen_MsgDownFramType;

typedef struct{
    uint32_t DevAddr;
    uint8_t fPort;
    uint8_t size;
	uint8_t *payload;
    struct
    {
        uint8_t AckRequest      : 1;
		uint8_t Ack             : 1;
        uint8_t Reserve         : 7;
    }CtrlBits;
}st_Data2Node;

typedef struct{
    uint32_t DevAddr;
}st_Confirm2Node;

typedef struct{
	en_MsgDownFramType enMsgDownFramType;
	union{
		st_Data2Node stData2Node;
		st_Confirm2Node stConfirm2Node;
	}Msg;
}st_ServerMsgDown,*pst_ServerMsgDown;

typedef struct{
	st_ServerMsgUp stServerMsgUp;
	struct list_head list;
}st_ServerMsgUpQueue,*pst_ServerMsgUpQueue;

typedef struct{
	st_ServerMsgUp stServerMsgDown;
	struct list_head list;
}st_ServerMsgDownQueue,*pst_ServerMsgDownQueue;

void ServerMsgInit(void);
void ServerMsgRemove(void);
int ServerMsgUpListGet(const pst_ServerMsgUp pstServerMsgUp);
int ServerMsgUpListAdd(const pst_ServerMsgUp pstServerMsgUp);
int ServerMsgDownListGet(pst_ServerMsgDown pstServerMsgDown);
void ServerMsgDownListAdd(pst_ServerMsgDown pstServerMsgDown);
int ServerMsgDownProcess(void *data);

#endif
