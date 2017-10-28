#ifndef __TYPE_DEF__
#define __TYPE_DEF__
#include <linux/stddef.h>
#include <linux/types.h>
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif


#define LORA_RX_PKG_MAX_SIZE    256
struct lora_rx_data{
    int chip;
    int len;
    unsigned char *buffer;
    struct list_head list;
};
extern struct lora_rx_data lora_rx_list;
extern bool rx_done;

#endif
