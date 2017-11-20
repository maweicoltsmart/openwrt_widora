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
}st_lora_rx_data_type,*pst_lora_rx_data_type;

typedef struct
{
	uint32_t jiffies_start;
	uint32_t jiffies_end;
	uint32_t chip;
	uint32_t len;
	struct timer_list *timer;
}st_lora_tx_data_type,*pst_lora_tx_data_type;

int register_sx1276_cdev(void);
int unregister_sx1276_cdev(void);

void OnTxDone( int chip );
void OnRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( int chip );
void OnRxTimeout( int chip );
void OnRxError( int chip );

#endif
