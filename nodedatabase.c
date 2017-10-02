#include "nodedatabase.h"

gateway_pragma_t gateway_pragma;
node_pragma_t nodebase_node_pragma[MAX_NODE];
static uint16_t node_cnt = 0;

uint16_t database_node_join(node_join_info_t* node)
{
	uint16_t loop;
	for(loop = 0;loop < node_cnt;loop ++)
	{
		if(!memcmp(node->DevEUI,nodebase_node_pragma[loop].DevEUI,8)){
			LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, gateway_pragma.AppNonce, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
			memcpy(nodebase_node_pragma[loop].DevNonce, node->DevNonce, 2);
			do_gettimeofday(&(nodebase_node_pragma[loop].LastComunication.time));
			*(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
			node_cnt ++;
			return loop;
		}
	}
	if(loop < MAX_NODE){
		memcpy(nodebase_node_pragma[loop].DevNonce, node->DevNonce, 2);
		memcpy(nodebase_node_pragma[loop].APPEUI, node->APPEUI, 8);
		memcpy(nodebase_node_pragma[loop].DevEUI, node->DevEUI, 8);
		do_gettimeofday(&(nodebase_node_pragma[loop].LastComunication.time));
		LoRaMacJoinComputeSKeys( gateway_pragma.APPKEY, gateway_pragma.AppNonce, node->DevNonce, nodebase_node_pragma[loop].NwkSKey, nodebase_node_pragma[loop].AppSKey );
		*(uint32_t*)nodebase_node_pragma[loop].DevAddr = loop;
		node_cnt ++;
	}
	return loop;
}

void *get_msg_to_send(uint32_t DevAddr)
{
	return NULL;
}
