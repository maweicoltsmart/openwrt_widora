#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include "debug.h"

unsigned char* enEventName[] = {
	[EV_JOIN_REQ] = "EV_JOIN_REQ",
	[EV_DATA_UNCONFIRMED_UP] = "EV_DATA_UNCONFIRMED_UP",
	[EV_DATA_CONFIRMED_UP] = "EV_DATA_CONFIRMED_UP",
	[EV_RXCOMPLETE] = "EV_RXCOMPLETE",
	[EV_TXCOMPLETE] = "EV_TXCOMPLETE",
	[EV_TX_START] = "EV_TX_START",
	[EV_DATA_PREPARE_TO_SEND] = "EV_DATA_PREPARE_TO_SEND",
};

void debug_output_data(uint8_t *data,int len)
{
	int loop;
	for(loop = 0;loop < len;loop ++)
	{
		printk("%02X ",data[loop]);
	}
	printk("\r\n");
}

