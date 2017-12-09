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
#include <linux/time.h>
#include <linux/timer.h>
#include "proc.h"
#include "LoRaMac.h"
#include "radiomsg.h"

#define LORADEV_IOC_MAGIC  'r'

#define LORADEV_IOCPRINT   _IO(LORADEV_IOC_MAGIC, 0)  //没参数
#define LORADEV_IOCGETDATA _IOR(LORADEV_IOC_MAGIC, 1, int)  //读
#define LORADEV_IOCSETDATA _IOW(LORADEV_IOC_MAGIC, 2, int)  //写
#define LORADEV_RADIO_INIT   _IOW(LORADEV_IOC_MAGIC, 3, int)  //没参数
#define LORADEV_RADIO_STATE _IOW(LORADEV_IOC_MAGIC, 4, int)  //读
#define LORADEV_RADIO_CHANNEL _IOW(LORADEV_IOC_MAGIC, 5, int)  //写
#define LORADEV_RADIO_SET_PUBLIC _IOW(LORADEV_IOC_MAGIC, 6, int)  //写
#define LORADEV_RADIO_SET_MODEM _IOW(LORADEV_IOC_MAGIC, 7, int)  //写
#define LORADEV_RADIO_READ_REG _IOWR(LORADEV_IOC_MAGIC, 8, int)  //写
#define LORADEV_RADIO_SET_TXCFG _IOW(LORADEV_IOC_MAGIC, 9, int)
#define LORADEV_RADIO_SET_RXCFG _IOW(LORADEV_IOC_MAGIC, 10, int)
#define LORADEV_RADIO_SET_RX _IOW(LORADEV_IOC_MAGIC, 11, int)
#define LORADEV_RADIO_SET_TX _IOW(LORADEV_IOC_MAGIC, 12, int)
#define LORADEV_RADIO_SET_SLEEP _IOW(LORADEV_IOC_MAGIC, 13, int)
#define LORADEV_RADIO_SET_STDBY _IOW(LORADEV_IOC_MAGIC, 14, int)

#define LORADEV_IOC_MAXNR 15

static struct fasync_struct *lora_node_event_button_fasync;

/* 定义并初始化等待队列头 */
static DECLARE_WAIT_QUEUE_HEAD(lora_wait);


static struct class *lora_dev_class;
static struct device *lora_dev_device;
static struct task_struct *radio_routin;

#define LORADEV_MAJOR 0   /*预设的mem的主设备号*/
#define LORADEV_NR_DEVS 2    /*设备数*/
#define LORADEV_SIZE 4096

static int lora_major = LORADEV_MAJOR;

module_param(lora_major, int, S_IRUGO);

struct cdev cdev;

/*!
 * Radio events function pointer
 */

bool isMaster = true;
bool rx_done = 0;
RadioEvents_t RadioEvents;


const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

struct lora_rx_data lora_rx_list;
struct lora_tx_data lora_tx_list;
/*
void OnTxDone( int chip )
{
    Radio.Sleep( chip);
    SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stChannel[chip].datarate,
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
    Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stChannel[chip].channel]);
    Radio.Rx( chip,0 );
    State = TX;
}

void OnRxDone( int chip,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    struct lora_rx_data *new;
    pst_lora_rx_data_type p;
    int len;
    uint8_t *pkg;
    printk("chip%d %s, %d bytes,jiffies = %d, HZ = %d\r\n",chip,__func__,size,(int)jiffies,(int)HZ);
    hexdump((const unsigned char *)payload,size);
    Radio.Sleep( chip);
    new = (struct lora_rx_data *)kmalloc(sizeof(struct lora_rx_data),GFP_KERNEL);
    if(!new)
    {
        Radio.Rx( chip,0 );
        return;
    }
    len = size + sizeof(st_lora_rx_data_type);
    new->buffer = kmalloc(len,GFP_KERNEL);
    if(!(new->buffer))
    {
        Radio.Rx( chip,0 );
        kfree(new);
        return;
    }
    p = (pst_lora_rx_data_type)new->buffer;
    pkg = (uint8_t*)&new->buffer[sizeof(st_lora_rx_data_type)];
    memcpy(pkg,payload,size);
    Radio.Rx( chip,0 );
    //printk("%s, %d \r\n",__func__,pkg[0]);
    p->jiffies = jiffies;
    p->chip = chip;
    p->len = size;
    new->len  = len;
    list_add_tail(&(new->list), &lora_rx_list.list);//使用尾插法
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
    rx_done = 1;
    wake_up(&lora_wait);
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
    Radio.Rx( chip,0 );
    State = RX_TIMEOUT;
}

void OnRxError( int chip )
{
    Radio.Sleep( chip);
    Radio.Rx( chip,RX_TIMEOUT_VALUE );
    State = RX_ERROR;
}*/

static int lora_dev_open(struct inode * inode, struct file * filp)
{
    RadioMsgListInit();
    RadioEvents.TxDone = OnMacTxDone;
    RadioEvents.RxDone = OnMacRxDone;
    RadioEvents.RxError = OnMacRxError;
    RadioEvents.TxTimeout = OnMacTxTimeout;
    RadioEvents.RxTimeout = OnMacRxTimeout;
    Radio.Init(0,&RadioEvents);
    Radio.Init(1,&RadioEvents);
    //Radio.Init(2,&RadioEvents);
    Radio.Sleep(0);
    Radio.Sleep(1);
    //Radio.Sleep(2);
    //SX1276IoIrqInit(0);
    //SX1276IoIrqInit(1);
    //Radio.Rx( 0,RX_TIMEOUT_VALUE );
    //Radio.Rx( 1,RX_TIMEOUT_VALUE );
    radio_routin = kthread_create(Radio_routin, NULL, "Radio1 routin thread");
    return 0;
}

static ssize_t lora_dev_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
    #if 0
    int ret = 0;
    struct lora_rx_data *get;
    struct list_head *pos;
    while (list_empty(&lora_rx_list.list)) /* 没有数据可读，考虑为什么不用if，而用while */
    {
        //printk("%s,%d\r\n",__func__,__LINE__);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        rx_done = 0;
        wait_event_interruptible(lora_wait,rx_done);
    }
    //printk("%s,%d\r\n",__func__,__LINE__);
    pos = lora_rx_list.list.next;
    get = list_entry(pos, struct lora_rx_data, list);
    if(!get)
    {
        return 0;
    }
    //printk("%s, %d, len = %d\r\n",__func__,__LINE__,get->len);
    /*读数据到用户空间*/
    if (copy_to_user(user, (void*)(get->buffer), get->len))
    {
        //printk("%s, %d, len = %d\r\n",__func__,__LINE__,get->len);
        ret =  - EFAULT;
    }
    else
    {
        //printk(KERN_INFO "read %d bytes(s) from list\n", get->len);
        //printk("%s, %d, len = %d\r\n",__func__,__LINE__,get->len);
        ret = get->len;
    }
    //printk("%s,%d\r\n",__func__,__LINE__);
    list_del(&get->list);
    if(get->len > 0)
    {
        kfree(get->buffer);
    }
    kfree(get);
    return ret;
    #endif
}

void lora_dev_tx_queue_routin( unsigned long data )
{
    #if 0
    pst_lora_tx_data_type p = (pst_lora_tx_data_type)data;
    uint8_t *buf = (uint8_t*)p;
    buf = &buf[sizeof(st_lora_tx_data_type)];
    Radio.Sleep(p->chip);
    SX1276SetTxConfig(p->chip,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stChannel[p->chip].datarate,
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
    Radio.SetChannel(p->chip,stRadioCfg_Tx.freq_tx[stChannel[p->chip].channel]);
    Radio.Send(p->chip,buf,p->len);
    /*p->timer->expires = jiffies + 10;
    add_timer(p->timer);*/
    del_timer(p->timer);
    kfree(p->timer);
    kfree(p);
    printk("chip%d send %d bytes: 0x",p->chip,p->len);
    hexdump((const unsigned char *)buf,p->len);
    #endif
}

static ssize_t lora_dev_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    #if 0
    pst_lora_tx_data_type p;
    struct timer_list *timer;
    timer = kmalloc(sizeof(struct timer_list),GFP_KERNEL);
    init_timer(timer);
    p = kmalloc(size,GFP_KERNEL);
    copy_from_user((void*)p, buf, size);
    timer->function = lora_dev_tx_queue_routin;
    timer->data = (unsigned long)p;
    timer->expires = p->jiffies_start;
    p->timer = timer;
    add_timer(timer);
    #endif
    return size;
#if 0
    struct lora_tx_data *new,*tmp;
    unsigned long a,b;
    pst_lora_tx_data_type p;
    p = kmalloc(size,GFP_KERNEL);
    if(!p)
    {
        return 0;
    }
    /*从用户空间写入数据*/
    copy_from_user(p, buf, size);
    new = (struct lora_tx_data *)kmalloc(sizeof(struct lora_tx_data),GFP_KERNEL);
    if(!new)
    {
        return 0;
    }
    new->buffer = (uint8_t*)p;
    new->chip = p->chip;
    new->len = p->len;
    new->jiffies = p->jiffies;
    a = new->jiffies;
    if(list_empty(&lora_tx_list.list))
        list_add_tail(&(new->list), &lora_tx_list.list);//使用尾插法
    else
    {
        list_for_each_entry(tmp, &lora_tx_list.list, list){
            b = tmp->jiffies;
            if(!time_after(a,b))
            {
                __list_add_rcu(&new->list,tmp->list.prev,&tmp->list);
                break;
            }
            else
            {
                if(list_is_last(&tmp->list,&lora_tx_list.list))
                {
                    list_add_tail(&(new->list), &lora_tx_list.list);//使用尾插法
                }
            }
        }
    }

    //Radio.Sleep(0);
    //Radio.Send(0,buffer,size);
    return size;
#endif
}

static int lora_dev_close(struct inode *inode, struct file *file)
{
    printk("%s,%d\r\n",__func__,__LINE__);

    Radio.Sleep(0);
    Radio.Sleep(1);
    //Radio.Sleep(2);

    del_timer(&TxTimeoutTimer[0]);
    del_timer(&RxTimeoutTimer[0]);
    //del_timer(&RxTimeoutSyncWord[0]);
    del_timer(&TxTimeoutTimer[1]);
    del_timer(&RxTimeoutTimer[1]);
    //del_timer(&RxTimeoutSyncWord[1]);
    //del_timer(&TxTimeoutTimer[2]);
    //del_timer(&RxTimeoutTimer[2]);
    //del_timer(&RxTimeoutSyncWord[2]);

    SX1276IoIrqFree(0);
    SX1276IoFree(0);
    SX1276IoIrqFree(1);
    SX1276IoFree(1);
    //SX1276IoIrqFree(2);
    //SX1276IoFree(2);
    kthread_stop(radio_routin);
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
    uint32_t ioarg = 0;
    int chip;
    uint32_t enable;
    uint32_t channel;
    uint32_t timeout;
    uint32_t datarate;
    //printk("%s,%d,cmd = %d\r\n",__func__,__LINE__,cmd);
    if (_IOC_TYPE(cmd) != LORADEV_IOC_MAGIC)
        return -EINVAL;
    //printk("%s,%d\r\n",__func__,__LINE__);
    if (_IOC_NR(cmd) > LORADEV_IOC_MAXNR)
        return -EINVAL;
    //printk("%s,%d\r\n",__func__,__LINE__);

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;
    //printk("%s,%d\r\n",__func__,__LINE__);

    switch(cmd) {
        case LORADEV_IOCPRINT:
        //printk("<--- CMD LORADEV_IOCPRINT Done--->\n\n");
        break;
        case LORADEV_IOCGETDATA:
        ioarg = 1101;
        ret = __put_user(ioarg, (int *)arg);
        break;
        case LORADEV_IOCSETDATA:
        ret = __get_user(ioarg, (int *)arg);
        //printk("<--- In Kernel LORADEV_IOCSETDATA ioarg = %d --->\n\n",ioarg);
        break;
        case LORADEV_RADIO_INIT:
        //printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        Radio.Init(chip,&RadioEvents);
        break;
        case LORADEV_RADIO_STATE:
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_PUBLIC:
        //printk("%s,%d\r\n",__func__,__LINE__);
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        enable = ioarg & 0x7fffffff;
        Radio.SetPublicNetwork(0,enable);
        Radio.SetPublicNetwork(1,enable);
        //Radio.SetPublicNetwork(2,enable);
        break;
        case LORADEV_RADIO_SET_MODEM:
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_CHANNEL:
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        channel = (ioarg & 0x0000ff00) >> 8;
        datarate = ioarg & 0x000000ff;
        stChannel[chip].channel = channel;
        stChannel[chip].datarate = datarate;
        //Radio.Sleep(chip);
        //Radio.SetChannel(chip,channel);
        //Radio.Rx( chip,0 );
        //printk("%s,%d,chip = %d, channel = %d,datarate = %d\r\n",__func__,__LINE__,chip,channel,datarate);
        break;
        case LORADEV_RADIO_SET_TXCFG:
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_RXCFG:
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_TX:
        /*ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        Radio.Tx( chip,0 );*/
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_RX:
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        timeout = ioarg & 0x7fffffff;
        SX1276SetTxConfig(chip,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stChannel[chip].datarate,
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
        SX1276SetRxConfig(chip,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stChannel[chip].datarate,
            stRadioCfg_Rx.coderate,
            stRadioCfg_Rx.bandwidthAfc,
            stRadioCfg_Rx.preambleLen,
            stRadioCfg_Rx.symbTimeout,
            stRadioCfg_Rx.fixLen,
            stRadioCfg_Rx.payloadLen,
            stRadioCfg_Rx.crcOn,
            stRadioCfg_Rx.freqHopOn,
            stRadioCfg_Rx.hopPeriod,
            stRadioCfg_Rx.iqInverted,
            stRadioCfg_Rx.rxContinuous);
        Radio.SetChannel(chip,stRadioCfg_Rx.freq_rx[stChannel[chip].channel]);
        Radio.Rx( chip,timeout );
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_SLEEP:
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        Radio.Sleep( chip);
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_SET_STDBY:
        ret = __get_user(ioarg, (int *)arg);
        chip = (ioarg & 0x80000000) >> 31;
        Radio.Standby( chip);
        //printk("%s,%d\r\n",__func__,__LINE__);
        break;
        case LORADEV_RADIO_READ_REG:
        ret = __get_user(ioarg, (int *)arg);
        ioarg = spi_w8r8(SX1276[0].Spi,ioarg & 0x7f);
        ret = __put_user(ioarg, (int *)arg);
        break;
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
