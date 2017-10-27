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
#include "sx1276-cdev.h"
#include "radio-default-param.h"
#include "sx1276-board.h"
#include <linux/list.h>

#define LORADEV_IOC_MAGIC  'r'

#define LORADEV_IOCPRINT            _IO(LORADEV_IOC_MAGIC, 0)  //没参数
#define LORADEV_IOCGETDATA          _IOR(LORADEV_IOC_MAGIC, 1, int)  //读
#define LORADEV_IOCSETDATA          _IOW(LORADEV_IOC_MAGIC, 2, int)  //写
#define LORADEV_RADIO_INIT          _IOW(LORADEV_IOC_MAGIC, 3, int)  //没参数
#define LORADEV_RADIO_STATE         _IOW(LORADEV_IOC_MAGIC, 4, int)  //读
#define LORADEV_RADIO_CHANNEL       _IOW(LORADEV_IOC_MAGIC, 5, int)  //写
#define LORADEV_RADIO_SET_PUBLIC    _IOW(LORADEV_IOC_MAGIC, 6, int)  //写
#define LORADEV_RADIO_SET_MODEM     _IOW(LORADEV_IOC_MAGIC, 7, int)  //写
#define LORADEV_RADIO_READ_REG      _IOWR(LORADEV_IOC_MAGIC, 8, int)  //写
#define LORADEV_RADIO_SET_TXCFG     _IOW(LORADEV_IOC_MAGIC, 9, int)
#define LORADEV_RADIO_SET_RXCFG     _IOW(LORADEV_IOC_MAGIC, 10, int)
#define LORADEV_IOC_MAXNR 11

static struct fasync_struct *lora_node_event_button_fasync;

/* 定义并初始化等待队列头 */
static DECLARE_WAIT_QUEUE_HEAD(lora_wait);


static struct class *lora_dev_class;
static struct device *lora_dev_device;
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

struct cdev cdev;

static RadioEvents_t RadioEvents;


const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

#define LORA_RX_PKG_MAX_SIZE    256
struct lora_rx_data{
    int chip;
    int len;
    unsigned char *buffer;
    struct list_head list;
};
struct list_head lora_rx_list;
/*!
 * Radio events function pointer
 */

bool isMaster = true;
static bool rx_done = 0;

void OnTxDone( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = TX;
}

void OnRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    struct lora_rx_data *new;
    Radio.Sleep( chip);
    new = (struct lora_rx_data *)kmalloc(sizeof(struct lora_rx_data),GFP_KERNEL);
    new->buffer = kmalloc(size,GFP_KERNEL);
    memcpy(new->buffer,payload,size);
    new->chip = chip;
    new->len = size;
    list_add_tail(&(new->list), &lora_rx_list);//使用尾插法
    RssiValue = rssi;
    SnrValue = snr;
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = RX;
    rx_done = 1;
}

void OnTxTimeout( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = TX_TIMEOUT;
}

void OnRxTimeout( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = RX_TIMEOUT;
}

void OnRxError( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = RX_ERROR;
}

static int lora_dev_open(struct inode * inode, struct file * filp)
{
    INIT_LIST_HEAD(&lora_rx_list);
    printk("%s,%d\r\n",__func__,__LINE__);
    return 0;
}

static ssize_t lora_dev_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
    int ret = 0;
    struct lora_rx_data *get;
    while (list_empty(&lora_rx_list)) /* 没有数据可读，考虑为什么不用if，而用while */
    {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        rx_done = 0;
        wait_event_interruptible(lora_wait,rx_done);
    }
    get = list_entry(&lora_rx_list, struct lora_rx_data, list);
    /*读数据到用户空间*/
    if (copy_to_user(user, (void*)(get->buffer), get->len))
    {
        ret =  - EFAULT;
    }
    else
    {
        printk(KERN_INFO "read %d bytes(s) from list\n", get->len);
        ret = get->len;
    }
    kfree(get->buffer);
    kfree(get);
    return ret;
}

static ssize_t lora_dev_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    uint8_t buffer[256];

    printk("%s,%d\r\n",__func__,__LINE__);

    /*从用户空间写入数据*/
    copy_from_user(buffer, buf, size);
    Radio.Sleep(0);
    Radio.Send(0,buffer,size);
    return size;
}

static int lora_dev_close(struct inode *inode, struct file *file)
{
    printk("%s,%d\r\n",__func__,__LINE__);

    SX1276IoIrqFree(0);
    SX1276IoFree(0);
    //kthread_stop(radio_routin);
    return 0;
}
static unsigned int lora_dev_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    printk("%s,%d\r\n",__func__,__LINE__);
    return mask;
}
static long lora_dev_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
    int err = 0;
    int ret = 0;
    int ioarg = 0;
    printk("%s,%d,cmd = %d\r\n",__func__,__LINE__,cmd);
    if (_IOC_TYPE(cmd) != LORADEV_IOC_MAGIC)
        return -EINVAL;
    printk("%s,%d\r\n",__func__,__LINE__);
    if (_IOC_NR(cmd) > LORADEV_IOC_MAXNR)
        return -EINVAL;
    printk("%s,%d\r\n",__func__,__LINE__);

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;
    printk("%s,%d\r\n",__func__,__LINE__);

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
        case LORADEV_RADIO_INIT:
        printk("%s,%d\r\n",__func__,__LINE__);
        // Initialize Radio driver
        RadioEvents.TxDone = OnTxDone;
        RadioEvents.RxDone = OnRxDone;
        RadioEvents.RxError = OnRxError;
        RadioEvents.TxTimeout = OnTxTimeout;
        RadioEvents.RxTimeout = OnRxTimeout;
        Radio.Init(0,&RadioEvents);
        break;
        case LORADEV_RADIO_STATE:
        printk("%s,%d\r\n",__func__,__LINE__);
        Radio.Sleep( 0);
        case LORADEV_RADIO_SET_PUBLIC:
        printk("%s,%d\r\n",__func__,__LINE__);
        // Random seed initialization
        //srand1( Radio.Random( ) );
        ret = __get_user(ioarg, (int *)arg);
        Radio.SetPublicNetwork( 0,ioarg );
        case LORADEV_RADIO_SET_MODEM:
        printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        Radio.SetModem(0,ioarg);
        break;
        case LORADEV_RADIO_CHANNEL:
        printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        Radio.SetChannel(0,ioarg);
        break;
        case LORADEV_RADIO_SET_TXCFG:
        printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        Radio.SetTxConfig( 0,MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
        break;
        case LORADEV_RADIO_SET_RXCFG:
        printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        Radio.SetRxConfig( 0,MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
        Radio.Rx( 0,RX_TIMEOUT_VALUE );
        break;
        case LORADEV_RADIO_READ_REG:
        ret = __get_user(ioarg, (int *)arg);
        ioarg = spi_w8r8(SX1276[0].Spi,ioarg & 0x7f);
        ret = __put_user(ioarg, (int *)arg);
        default:
        printk("%s,%d\r\n",__func__,__LINE__);
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
    .write      = lora_dev_write,
    .release    = lora_dev_close,
    .poll       = lora_dev_poll,
    .fasync     = lora_dev_fasync,
    .unlocked_ioctl      = lora_dev_ioctl,
};

int register_sx1276_cdev(void)
{
    dev_t devno = MKDEV(lora_major, 0);
    int ret;
    /* 静态申请设备号*/
    if (lora_major)
    ret = register_chrdev_region(devno, LORADEV_NR_DEVS, "lora_dev");
    else  /* 动态分配设备号 */
    {
        ret = alloc_chrdev_region(&devno, 0, LORADEV_NR_DEVS, "lora_dev");
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
    lora_dev_class = class_create(THIS_MODULE, "lora_dev_class");
    lora_dev_device = device_create(lora_dev_class, NULL, MKDEV(lora_major, 0), NULL, "lora_radio");

    return 0;
}

int  unregister_sx1276_cdev(void)
{
    int ret = 0;
    device_unregister(lora_dev_device);  //卸载类下的设备
    class_destroy(lora_dev_class);      //卸载类
    cdev_del(&cdev);   /*注销设备*/
    unregister_chrdev_region(MKDEV(lora_major, 0), 2); /*释放设备号*/
    return ret;
}
