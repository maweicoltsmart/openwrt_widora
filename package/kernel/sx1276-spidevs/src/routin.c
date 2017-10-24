#include "radio.h"
#include "routin.h"
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include "utilities.h"
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/module.h> //Needed by all modules
#include <linux/kernel.h> //Needed for KERN_ALERT

/**
 * Main application entry point.
 */
int Radio_routin(void *data){
    int j,k;
    int timeout;
    wait_queue_head_t timeout_wq;
    static int i = 0;
    i++;
    j = 0;
    k = i;
    printk("thread_func %d started/n", i);
    init_waitqueue_head(&timeout_wq);
    while(!kthread_should_stop())
    {
        wait_event_interruptible_timeout(timeout_wq, 0,HZ);
        //printk("[%d]sleeping..%d/n", k, j++);
    }
    printk("%s going stop\r\n",__func__);
    return 0;
}
