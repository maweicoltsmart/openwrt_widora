#include "radio.h"
#include "routin-1.h"
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include "LoRaMac.h"

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here

static int event1 = 0;
DECLARE_WAIT_QUEUE_HEAD(wq1);

/*!
 * Radio_1 events function pointer
 */
//static RadioEvents_t Radio_1Events;
static LoRaMacCallbacks_t LoRaMac_1_Callbacks;
/*!
 * \brief Function to be executed on Radio_1 Tx Done event
 */

int Radio_1_routin(void *data){
	bool isMaster = true;
    uint8_t i;
    int err;

	LoRaMacInit(&LoRaMac_1_Callbacks);
    msleep(100);
	
	while(1)
	{
        //printk("%s,%d\r\n",__func__,__LINE__);
        wait_event(wq1, event1 == 1);
        event1 = 0;

	}
	return 0;
}