#include "routin.h"
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include "si4438-board.h"
#include "radio.h"

int event1 = 0;
DECLARE_WAIT_QUEUE_HEAD(wq1);
extern SEGMENT_VARIABLE(bMain_IT_Status, U8, SEG_XDATA);
void DemoApp_Pollhandler(void);
bool gSampleCode_SendVariablePacket(void);

int Radio_routin(void *data){
	si4438_IoInit();
	si4438_IoIrqInit();
	vRadio_Init();

	//vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber,0u);
	gSampleCode_SendVariablePacket();
	while(1)
	{
        //printk("%s,%d\r\n",__func__,__LINE__);
        wait_event(wq1, event1 == 1);
        event1 = 0;
        
        DemoApp_Pollhandler();
	}
	return 0;
}