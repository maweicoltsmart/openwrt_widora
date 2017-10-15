#include "radio.h"
#include "routin.h"
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include "utilities.h"

/**
 * Main application entry point.
 */
int Radio_routin(void *data){
	while( 1 )
    {

    }
	return 0;
}
