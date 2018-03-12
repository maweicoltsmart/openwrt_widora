#ifndef __SX1276_CDEV_H__
#define __SX1276_CDEV_H__

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define LORA_RX_PKG_MAX_SIZE    256

extern wait_queue_head_t read_from_user_space_wait;
extern bool have_data;

int register_sx1276_cdev(void);
int unregister_sx1276_cdev(void);

#endif
