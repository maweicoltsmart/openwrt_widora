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

int register_sx1276_cdev(void);
int unregister_sx1276_cdev(void);

void OnTxDone( int chip );
void OnRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( int chip );
void OnRxTimeout( int chip );
void OnRxError( int chip );

#endif
