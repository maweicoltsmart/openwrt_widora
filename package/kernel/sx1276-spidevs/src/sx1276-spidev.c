/*
 *  Custom GPIO-based SPI driver
 *
 *  Copyright (C) 2013 Marco Burato <zmaster.adsl@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  Based on i2c-gpio-custom by:
 *  Copyright (C) 2007-2008 Gabor Juhos <juhosg@openwrt.org>
 * ---------------------------------------------------------------------------
 *
 *  The behaviour of this driver can be altered by setting some parameters
 *  from the insmod command line.
 *
 *  The following parameters are adjustable:
 *
 *	bus0	These four arguments can be arrays of
 *	bus1	1-8 unsigned integers as follows:
 *	bus2
 *	bus3	<id>,<sck>,<mosi>,<miso>,<mode1>,<maxfreq1>,<cs1>,...
 *
 *  where:
 *
 *  <id>	ID to used as device_id for the corresponding bus (required)
 *  <sck>	GPIO pin ID to be used for bus SCK (required)
 *  <mosi>	GPIO pin ID to be used for bus MOSI (required*)
 *  <miso>	GPIO pin ID to be used for bus MISO (required*)
 *  <modeX>	Mode configuration for slave X in the bus (required)
 *		(see /include/linux/spi/spi.h)
 *  <maxfreqX>	Maximum clock frequency in Hz for slave X in the bus (required)
 *  <csX>	GPIO pin ID to be used for slave X CS (required**)
 *
 *	Notes:
 *	*	If a signal is not used (for example there is no MISO) you need
 *		to set the GPIO pin ID for that signal to an invalid value.
 *	**	If you only have 1 slave in the bus with no CS, you can omit the
 *		<cs1> param or set it to an invalid GPIO id to disable it. When
 *		you have 2 or more slaves, they must all have a valid CS.
 *
 *  If this driver is built into the kernel, you can use the following kernel
 *  command line parameters, with the same values as the corresponding module
 *  parameters listed above:
 *
 *	sx1276-spidevs.bus0
 *	sx1276-spidevs.bus1
 *	sx1276-spidevs.bus2
 *	sx1276-spidevs.bus3
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>         //class_create
#include <linux/poll.h>   //poll  
#include <linux/fcntl.h>  
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/interrupt.h> //---request_irq()
#include <asm/irq.h> //---disable_irq, enable_irq()
#include <linux/workqueue.h>
#include "pinmap.h"
#include <linux/delay.h>
#include "routin.h"
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()、kthread_run()
#include <linux/err.h> //IS_ERR()、PTR_ERR()
#include <linux/spinlock.h>

DEFINE_SPINLOCK(spi_lock);

//#define DRV_NAME	"semtech,sx1278-1"
#define DRV_DESC	"LoraWAN Driver"
#define DRV_VERSION	"0.1"

//#define PFX		DRV_NAME ": "

static struct platform_device *devices;
struct spi_device *slave = NULL;

static struct task_struct *radio_routin;

static unsigned int sx1278_dio0irq,sx1278_dio1irq,sx1278_dio2irq,sx1278_dio3irq,sx1278_dio4irq,sx1278_dio5irq;

static int sx1276_spidevs_remove(struct spi_device *spi);
static int sx1276_spidevs_probe(struct spi_device *spi);

static void sx1276_spidevs_cleanup(void)
{
	int i;
	kthread_stop(radio_routin);

	free_irq(sx1278_dio0irq,NULL);
	free_irq(sx1278_dio1irq,NULL);
	free_irq(sx1278_dio2irq,NULL);
	free_irq(sx1278_dio3irq,NULL);
	free_irq(sx1278_dio4irq,NULL);
	free_irq(sx1278_dio5irq,NULL);

	gpio_free(SX1278_1_RST_PIN);
	gpio_free(SX1278_1_DIO0_PIN);
	gpio_free(SX1278_1_DIO1_PIN);
	gpio_free(SX1278_1_DIO2_PIN);
	gpio_free(SX1278_1_DIO3_PIN);
	gpio_free(SX1278_1_DIO4_PIN);
	gpio_free(SX1278_1_DIO5_PIN);
}

static int sx1276_spidevs_remove(struct spi_device *spi)
{
	sx1276_spidevs_cleanup();
	return 0;
}
static int sx1276_spidevs_probe(struct spi_device *spi)
{
	int err;
	slave = spi;
	printk("%s, %d,slave[0] = %d, slave[1] = %d\r\n",__func__,__LINE__,slave[0],slave[1]);
	printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
	spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    spi_setup(spi);
	printk("%s, %d\r\n",__func__,__LINE__);
	radio_routin = kthread_create(Radio_routin, NULL, "Radio1 routin thread");
	if(IS_ERR(radio_routin)){
		printk("Unable to start kernel thread radio_routin./n");  
		err = PTR_ERR(radio_routin);  
		radio_routin = NULL;  
		return err;
	}
	printk("%s, %d\r\n",__func__,__LINE__);
	wake_up_process(radio_routin);
	
	return 0;

err:
	sx1276_spidevs_cleanup();
	return err;
}

static const struct of_device_id lorawan_dt_ids[] = {
        { .compatible = "semtech,sx1278-1" },
        {},
};

MODULE_DEVICE_TABLE(of, lorawan_dt_ids);

static struct spi_driver lorawan_spi_driver = {
        .driver = {
                .name =         "semtech,sx1278-1",
                .owner =        THIS_MODULE,
                .of_match_table = of_match_ptr(lorawan_dt_ids),
        },
        .probe =        sx1276_spidevs_probe,
        .remove =       sx1276_spidevs_remove,

        /* NOTE:  suspend/resume methods are not necessary here.
         * We don't do anything except pass the requests to/from
         * the underlying controller.  The refrigerator handles
         * most issues; the controller driver handles the rest.
         */
};

int major;  
  
static struct fasync_struct *lora_node_event_button_fasync;  

/* 定义并初始化等待队列头 */  
static DECLARE_WAIT_QUEUE_HEAD(lora);  
  
  
static struct class *lora_node_event_drv_class;  
static struct device *lora_node_event_drv_device;  

static int lora_node_event_drv_open(struct inode * inode, struct file * filp)  
{  
    /*  K1 ---- EINT1,K2 ---- EINT4,K3 ---- EINT2,K4 ---- EINT0 
     *  配置GPF1、GPF4、GPF2、GPF0为相应的外部中断引脚 
     *  IRQT_BOTHEDGE应该改为IRQ_TYPE_EDGE_BOTH 
     */  
    return 0;  
}  
  
static ssize_t lora_node_event_drv_read(struct file *file, char __user *user, size_t size,loff_t *ppos)  
{  
    if (size != 1)  
            return -EINVAL;  
    char key_val = 'k';
    /* 当没有按键按下时，休眠。 
     * 即ev_press = 0; 
     * 当有按键按下时，发生中断，在中断处理函数会唤醒 
     * 即ev_press = 1;  
     * 唤醒后，接着继续将数据通过copy_to_user函数传递给应用程序 
     */  
    copy_to_user(user, &key_val, 1);  
      
    return 1;     
}  
  
static int lora_node_event_drv_close(struct inode *inode, struct file *file)  
{  
    return 0;  
}  
  
static unsigned int lora_node_event_drv_poll(struct file *file, poll_table *wait)  
{  
    unsigned int mask = 0;  
  
    return mask;    
}  
  
/* 当应用程序调用了fcntl(fd, F_SETFL, Oflags | FASYNC);  
 * 则最终会调用驱动的fasync函数，在这里则是fifth_drv_fasync 
 * fifth_drv_fasync最终又会调用到驱动的fasync_helper函数 
 * fasync_helper函数的作用是初始化/释放fasync_struct 
 */  
static int lora_node_event_drv_fasync(int fd, struct file *filp, int on)  
{  
    return fasync_helper(fd, filp, on, &lora_node_event_button_fasync);  
}  

/* File operations struct for character device */  
static const struct file_operations lora_node_event_drv_fops = {  
    .owner      = THIS_MODULE,  
    .open       = lora_node_event_drv_open,  
    .read       = lora_node_event_drv_read,  
    .release    = lora_node_event_drv_close,  
    .poll       = lora_node_event_drv_poll,  
    .fasync     = lora_node_event_drv_fasync,  
};  

static int __init lorawan_init(void)
{
	int ret;
	printk("%s, %d\r\n",__func__,__LINE__);	
	ret = spi_register_driver(&lorawan_spi_driver);
	
	printk("%s, %d\r\n",__func__,__LINE__);

	    /* 主设备号设置为0表示由系统自动分配主设备号 */  
    major = register_chrdev(0, "lora_node_event", &lora_node_event_drv_fops);  
  
    /* 创建fifthdrv类 */  
    lora_node_event_drv_class = class_create(THIS_MODULE, "lora_node_event");  
  
    /* 在fifthdrv类下创建buttons设备，供应用程序打开设备*/  
    lora_node_event_drv_device = device_create(lora_node_event_drv_class, NULL, MKDEV(major, 0), NULL, "lora_radio");  
  
	return ret;
}

static void __exit lorawan_exit(void)
{
	kthread_stop(radio_routin);
	printk("%s, %d\r\n",__func__,__LINE__);
	spi_unregister_driver(&lorawan_spi_driver);
	
	printk("%s, %d\r\n",__func__,__LINE__);
	unregister_chrdev(major, "lora_node_event");  
    device_unregister(lora_node_event_drv_device);  //卸载类下的设备  
    class_destroy(lora_node_event_drv_class);      //卸载类  
}

module_init(lorawan_init);
module_exit(lorawan_exit);

MODULE_AUTHOR("HorseMa, <wei_technology@163.com>");
MODULE_DESCRIPTION("LoraWAN small gateway use two chips sx1278");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" "semtech,sx1278");
