#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/timer.h>
#include "proc.h"
#include "LoRaWAN.h"
#include "sx1276-cdev.h"

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
DECLARE_WAIT_QUEUE_HEAD(read_from_user_space_wait);


static struct class *lora_dev_class;
static struct device *lora_dev_device;

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
bool have_data = false;

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

static int lora_dev_open(struct inode * inode, struct file * filp)
{
	LoRaWANInit();
    return 0;
}

static ssize_t lora_dev_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
    int ret = 0;
    uint8_t buffer[256 + sizeof(st_ServerMsgUp)];
	uint32_t len;
	
	pst_ServerMsgUp pstServerMsgUp = (pst_ServerMsgUp)buffer;
	pstServerMsgUp->Msg.stData2Server.payload = buffer + sizeof(st_ServerMsgUp);
    while ((ret = ServerMsgUpListGet(pstServerMsgUp)) < 0)
    {
        if (filp->f_flags & O_NONBLOCK)
        {
            return -EAGAIN;
        }
        have_data = false;
        wait_event_interruptible(read_from_user_space_wait,have_data);
    }
	if(pstServerMsgUp->enMsgUpFramType == en_MsgUpFramDataReceive)
	{
		len = pstServerMsgUp->Msg.stData2Server.size + sizeof(st_ServerMsgUp);
		ret = len;
	}
	else
	{
		len = sizeof(st_ServerMsgUp);
		ret = len;
	}
    if (copy_to_user(user, (void*)buffer, len))
    {
        ret =  - EFAULT;
    }
    return ret;
}

void lora_dev_tx_queue_routin( unsigned long data )
{

}

static ssize_t lora_dev_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	uint8_t buffer[256 + sizeof(st_ServerMsgDown)];
	pst_ServerMsgDown pstServerMsgDown = (pst_ServerMsgDown)buffer;
	
	copy_from_user((void*)pstServerMsgDown, buf, size);
	if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramDataSend)
	{
		pstServerMsgDown->Msg.stData2Node.payload = buffer + sizeof(st_ServerMsgDown);
		ServerMsgDownListAdd(pstServerMsgDown);
	}
	else if(pstServerMsgDown->enMsgDownFramType == en_MsgDownFramConfirm)
	{
		ServerMsgDownListAdd(pstServerMsgDown);
	}
	else
	{
	}
	return size;
}

static int lora_dev_close(struct inode *inode, struct file *file)
{
	LoRaWANRemove();
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
    //uint32_t ioarg = 0;
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

        default:
        return -EINVAL;
    }
    return ret;
}

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
