#ifndef __TYPE_DEF__
#define __TYPE_DEF__
#include <linux/stddef.h>
#include <linux/types.h>
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif


#define LORA_RX_PKG_MAX_SIZE    256
struct lora_rx_data{
    uint32_t len;
    uint8_t *buffer;
    struct list_head list;
};
extern struct lora_rx_data lora_rx_list;

struct lora_tx_data{
	uint32_t jiffies;
	uint32_t chip;
	uint32_t len;
	uint8_t *buffer;
	struct list_head list;
};
extern struct lora_tx_data lora_tx_list;

typedef struct
{
	uint32_t jiffies;
	uint32_t chip;
	uint32_t len;
	uint8_t *buffer;
}st_lora_tx_data_type,*pst_lora_tx_data_type,st_lora_rx_data_type,*pst_lora_rx_data_type;
extern bool rx_done;

#endif
