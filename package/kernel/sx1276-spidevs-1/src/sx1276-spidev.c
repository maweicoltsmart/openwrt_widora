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
#include <linux/cdev.h>  
#include "radio.h"
#include "sx1276.h"

DEFINE_SPINLOCK(spi_lock);

//#define DRV_NAME	"semtech,sx1278-1"
#define DRV_DESC	"LoraWAN Driver"
#define DRV_VERSION	"0.1"

//#define PFX		DRV_NAME ": "

#define LORADEV_IOC_MAGIC  'l'

#define LORADEV_IOCPRINT   _IO(LORADEV_IOC_MAGIC, 1)  //没参数
#define LORADEV_IOCGETDATA _IOR(LORADEV_IOC_MAGIC, 2, int)  //读
#define LORADEV_IOCSETDATA _IOW(LORADEV_IOC_MAGIC, 3, int)  //写

#define LORADEV_IOC_MAXNR 3

static struct platform_device *devices;
struct spi_device *slave = NULL;

static struct task_struct *radio_routin;

static unsigned int sx1278_dio0irq,sx1278_dio1irq,sx1278_dio2irq,sx1278_dio3irq,sx1278_dio4irq,sx1278_dio5irq;

static int sx1276_spidevs_remove(struct spi_device *spi);
static int sx1276_spidevs_probe(struct spi_device *spi);
static RadioEvents_t RadioEvents;
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

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

/*!
 * Radio events function pointer
 */

bool isMaster = true;

void OnTxDone( void )
{
    Radio.Sleep( );
    State = TX;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    State = TX_TIMEOUT;
}

void OnRxTimeout( void )
{
    Radio.Sleep( );
    State = RX_TIMEOUT;
}

void OnRxError( void )
{
    Radio.Sleep( );
    State = RX_ERROR;
}

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

static struct fasync_struct *lora_node_event_button_fasync;  

/* 定义并初始化等待队列头 */  
static DECLARE_WAIT_QUEUE_HEAD(lora);  
  
  
static struct class *lora_dev_class;  
static struct device *lora_dev_device;  

static int lora_dev_open(struct inode * inode, struct file * filp)  
{  
    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    #define RF_FREQUENCY								470000000 // Hz
    #define TX_OUTPUT_POWER 							14		  // dBm
    #define LORA_BANDWIDTH								0		  // [0: 125 kHz,
																  //  1: 250 kHz,
																  //  2: 500 kHz,
																  //  3: Reserved]
    #define LORA_SPREADING_FACTOR						7		  // [SF7..SF12]
    #define LORA_CODINGRATE 							1		  // [1: 4/5,
																  //  2: 4/6,
																  //  3: 4/7,
																  //  4: 4/8]
    #define LORA_PREAMBLE_LENGTH						8		  // Same for Tx and Rx
    #define LORA_SYMBOL_TIMEOUT 						5		  // Symbols
    #define LORA_FIX_LENGTH_PAYLOAD_ON					false
    #define LORA_IQ_INVERSION_ON						false

    Radio.Init( &RadioEvents );

    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    return 0;   
}  

static ssize_t lora_dev_read(struct file *file, char __user *user, size_t size,loff_t *ppos)  
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

static int lora_dev_close(struct inode *inode, struct file *file)  
{
    return 0;
}
static int lora_dev_poll(struct file *file, poll_table *wait)  
{
    unsigned int mask = 0;
    return mask;
}
static int lora_dev_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;  
    int ret = 0;  
    int ioarg = 0;  
      
      
    if (_IOC_TYPE(cmd) != LORADEV_IOC_MAGIC)   
        return -EINVAL;  
    if (_IOC_NR(cmd) > LORADEV_IOC_MAXNR)   
        return -EINVAL;  
  
      
    if (_IOC_DIR(cmd) & _IOC_READ)  
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));  
    else if (_IOC_DIR(cmd) & _IOC_WRITE)  
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));  
    if (err)   
        return -EFAULT;  
  
      
    switch(cmd) {  
  
        
      case LORADEV_IOCPRINT:  
        printk("<--- CMD LORADEV_IOCPRINT Done--->\n\n");  
        break;  
        
        
      case LORADEV_IOCGETDATA:   
        ioarg = 1101;  
        ret = __put_user(ioarg, (int *)arg);  
        break;  
        
        
      case LORADEV_IOCSETDATA:   
        ret = __get_user(ioarg, (int *)arg);  
        printk("<--- In Kernel LORADEV_IOCSETDATA ioarg = %d --->\n\n",ioarg);  
        break;  
  
      default:    
        return -EINVAL;  
    }  
    return ret;  
}

/* 当应用程序调用了fcntl(fd, F_SETFL, Oflags | FASYNC);  
 * 则最终会调用驱动的fasync函数，在这里则是fifth_drv_fasync 
 * fifth_drv_fasync最终又会调用到驱动的fasync_helper函数 
 * fasync_helper函数的作用是初始化/释放fasync_struct 
 */
static int lora_dev_fasync(int fd, struct file *filp, int on)  
{
    return fasync_helper(fd, filp, on, &lora_node_event_button_fasync);  
}

/* File operations struct for character device */  
static const struct file_operations lora_dev_fops = {  
    .owner      = THIS_MODULE,
    .open       = lora_dev_open,
    .read       = lora_dev_read,
    .release    = lora_dev_close,
    .poll       = lora_dev_poll,
    .fasync     = lora_dev_fasync,
    .unlocked_ioctl      = lora_dev_ioctl,
};
#define LORADEV_MAJOR 0   /*预设的mem的主设备号*/  
#define LORADEV_NR_DEVS 2    /*设备数*/  
#define LORADEV_SIZE 4096  
/*mem设备描述结构体*/  
struct lora_dev                                       
{                                                          
  char *data;                        
  unsigned long size;         
};  
static int lora_major = LORADEV_MAJOR;  
  
module_param(lora_major, int, S_IRUGO);  
  
struct lora_dev *lora_devp; /*设备结构体指针*/  
  
struct cdev cdev;   

static int sx1276_spidevs_remove(struct spi_device *spi)
{
	sx1276_spidevs_cleanup();
	#if 1
    device_unregister(lora_dev_device);  //卸载类下的设备  
    class_destroy(lora_dev_class);      //卸载类 
    cdev_del(&cdev);   /*注销设备*/  
	kfree(lora_devp);	 /*释放设备结构体内存*/  
	unregister_chrdev_region(MKDEV(lora_major, 0), 2); /*释放设备号*/  
	#endif
	return 0;
}
static int sx1276_spidevs_probe(struct spi_device *spi)
{
	int err;
	int ret;
	int i;
	slave = spi;
	//printk("%s, %d,slave[0] = %d, slave[1] = %d\r\n",__func__,__LINE__,slave[0],slave[1]);
	printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
	spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    spi_setup(spi);
	printk("%s, %d\r\n",__func__,__LINE__);
    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
#define RF_FREQUENCY								470000000 // Hz
#define TX_OUTPUT_POWER 							14		  // dBm
#define LORA_BANDWIDTH								0		  // [0: 125 kHz,
																  //  1: 250 kHz,
																  //  2: 500 kHz,
																  //  3: Reserved]
#define LORA_SPREADING_FACTOR						7		  // [SF7..SF12]
#define LORA_CODINGRATE 							1		  // [1: 4/5,
																  //  2: 4/6,
																  //  3: 4/7,
																  //  4: 4/8]
#define LORA_PREAMBLE_LENGTH						8		  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 						5		  // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON					false
#define LORA_IQ_INVERSION_ON						false

    Radio.Init( &RadioEvents );

    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
	radio_routin = kthread_create(Radio_routin, NULL, "Radio1 routin thread");
	if(IS_ERR(radio_routin)){
		printk("Unable to start kernel thread radio_routin./n");  
		err = PTR_ERR(radio_routin);  
		radio_routin = NULL;  
		return err;
	}
	printk("%s, %d\r\n",__func__,__LINE__);
	wake_up_process(radio_routin);
	#if 1
	dev_t devno = MKDEV(lora_major, 0);	
	
	/* 静态申请设备号*/	
	if (lora_major)	
	  ret = register_chrdev_region(devno, LORADEV_NR_DEVS, "lora_dev_1");	
	else  /* 动态分配设备号 */  
	{  
	  ret = alloc_chrdev_region(&devno, 0, LORADEV_NR_DEVS, "lora_dev_1");  
	  lora_major = MAJOR(devno);  
	}	 
	  
	if (ret < 0)  
	  return ret;  
	
	/*初始化cdev结构*/  
	cdev_init(&cdev, &lora_dev_fops);//使cdev与mem_fops联系起来  
	cdev.owner = THIS_MODULE;//owner成员表示谁拥有这个驱动程序，使“内核引用模块计数”加1；THIS_MODULE表示现在这个模块被内核使用，这是内核定义的一个宏  
	cdev.ops = &lora_dev_fops;  
	  
	/* 注册字符设备 */  
	cdev_add(&cdev, MKDEV(lora_major, 0), LORADEV_NR_DEVS);  
	lora_dev_class = class_create(THIS_MODULE, "lora_dev_class_1");
	lora_dev_device = device_create(lora_dev_class, NULL, MKDEV(lora_major, 0), NULL, "lora_radio_1"); 
	/* 为设备描述结构分配内存*/	
	lora_devp = kmalloc(LORADEV_NR_DEVS * sizeof(struct lora_dev), GFP_KERNEL);//目前为止我们始终用GFP_KERNEL  
	if (!lora_devp)	  /*申请失败*/	
	{  
	  ret =	- ENOMEM;  
	  goto fail_malloc;  
	}  
	memset(lora_devp, 0, sizeof(struct lora_dev));  
	  
	/*为设备分配内存*/  
	for (i=0; i < LORADEV_NR_DEVS; i++)	 
	{  
		  lora_devp[i].size = LORADEV_SIZE;  
		  lora_devp[i].data = kmalloc(LORADEV_SIZE, GFP_KERNEL);//分配出来的地址存在此	
		  memset(lora_devp[i].data, 0, LORADEV_SIZE);  
	}  
		
	return 0;  
	
fail_malloc:   
	unregister_chrdev_region(devno, 1);  
	  #endif
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

static int __init lorawan_init(void)
{
	int ret;
	printk("%s, %d\r\n",__func__,__LINE__);	
	ret = spi_register_driver(&lorawan_spi_driver);
	
	printk("%s, %d\r\n",__func__,__LINE__);
	return ret;	
}

static void __exit lorawan_exit(void)
{
	sx1276_spidevs_cleanup();
	kthread_stop(radio_routin);
	printk("%s, %d\r\n",__func__,__LINE__);
	spi_unregister_driver(&lorawan_spi_driver);
}

module_init(lorawan_init);
module_exit(lorawan_exit);

MODULE_AUTHOR("HorseMa, <wei_technology@163.com>");
MODULE_DESCRIPTION("LoraWAN small gateway use two chips sx1278");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" "semtech,sx1278");
