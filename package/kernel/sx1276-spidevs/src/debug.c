#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include "debug.h"

void debug_output_data(uint8_t *data,int len)
{
	int loop;
	for(loop = 0;loop < len;loop ++)
	{
		printk("%02X ",data[loop]);
	}
	printk("\r\n");
}