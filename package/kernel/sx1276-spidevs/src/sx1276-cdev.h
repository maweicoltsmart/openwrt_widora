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

int register_sx1276_cdev(void);
int unregister_sx1276_cdev(void);

#endif