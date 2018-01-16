#include "proc.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>


#define MODULE_VERS "1.0"
#define MODULE_NAME "lora_procfs"

static struct proc_dir_entry *lora_dir, *lora_cfg_rx_file,*lora_cfg_tx_file;

st_RadioCfg stRadioCfg_Tx,stRadioCfg_Rx;

static int mytest_flag = 0;//create flag variable
static int mytest_proc_show(struct seq_file *seq,void *v)
{
    seq_puts(seq,mytest_flag ?"True\n":"False\n");
    return 0;
}

static int lora_cfg_rx_proc_open(struct inode *inode,struct file *file)
{
    return single_open(file,mytest_proc_show,inode->i_private);
}

static int proc_write_lora_cfg_rx(struct file *file, const char *buffer, size_t len, loff_t *off)
{
    int count;
    //printk("%s,%d\r\n",__func__,__LINE__);
    if(len > sizeof(st_RadioCfg))
        count = sizeof(st_RadioCfg);
    else
        count = len;
    //printk("%s,%d\r\n",__func__,__LINE__);
    memset(&stRadioCfg_Rx,0,sizeof(st_RadioCfg));
    if(copy_from_user((void*)&stRadioCfg_Rx, buffer, count))
        return -EFAULT;
/*
    printk("%s:\
            stRadioCfg_Rx.modem = %d,\
            stRadioCfg_Rx.bandwidth = %d,\
            stRadioCfg_Rx.datarate = %d,\
            stRadioCfg_Rx.freq_rx[0] = %d,\
            stRadioCfg_Rx.coderate = %d,\
            stRadioCfg_Rx.bandwidthAfc = %d,\
            stRadioCfg_Rx.preambleLen = %d,\
            stRadioCfg_Rx.symbTimeout = %d,\
            stRadioCfg_Rx.fixLen = %d,\
            stRadioCfg_Rx.payloadLen = %d,\
            stRadioCfg_Rx.crcOn = %d,\
            stRadioCfg_Rx.freqHopOn = %d,\
            stRadioCfg_Rx.hopPeriod = %d,\
            stRadioCfg_Rx.iqInverted = %d,\
            stRadioCfg_Rx.rxContinuous = %d\r\n",
            __func__,
            stRadioCfg_Rx.modem,
            stRadioCfg_Rx.bandwidth,
            stRadioCfg_Rx.datarate[0],
            stRadioCfg_Rx.freq_rx[0],
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
*/
    return len;
}

static const struct file_operations lora_cfg_rx_file_ops ={
    .owner = THIS_MODULE,
    .open = lora_cfg_rx_proc_open,
    .read = seq_read,
    .write = proc_write_lora_cfg_rx,
};

static int lora_cfg_tx_proc_open(struct inode *inode,struct file *file)
{
    return single_open(file,mytest_proc_show,inode->i_private);
}

static int proc_write_lora_cfg_tx(struct file *file, const char *buffer, size_t len, loff_t *off)
{
    int count;
    //printk("%s,%d\r\n",__func__,__LINE__);
    if(len > sizeof(st_RadioCfg))
        count = sizeof(st_RadioCfg);
    else
        count = len;
    //printk("%s,%d\r\n",__func__,__LINE__);
    memset(&stRadioCfg_Tx,0,sizeof(st_RadioCfg));
    if(copy_from_user((void*)&stRadioCfg_Tx, buffer, count))
        return -EFAULT;
/*
    printk("%s:\
            stRadioCfg_Tx.modem = %d,\
            stRadioCfg_Tx.power = %d,\
            stRadioCfg_Tx.fdev = %d,\
            stRadioCfg_Tx.bandwidth = %d,\
            stRadioCfg_Tx.datarate = %d,\
            stRadioCfg_Tx.freq_tx[0] = %d,\
            stRadioCfg_Tx.coderate = %d,\
            stRadioCfg_Tx.preambleLen = %d,\
            stRadioCfg_Tx.fixLen = %d,\
            stRadioCfg_Tx.crcOn = %d,\
            stRadioCfg_Tx.freqHopOn = %d,\
            stRadioCfg_Tx.hopPeriod = %d,\
            stRadioCfg_Tx.iqInverted = %d,\
            stRadioCfg_Tx.timeout = %d\r\n",
            __func__,
            stRadioCfg_Tx.modem,
            stRadioCfg_Tx.power,
            stRadioCfg_Tx.fdev,
            stRadioCfg_Tx.bandwidth,
            stRadioCfg_Tx.datarate[0],
            stRadioCfg_Tx.freq_tx[0],
            stRadioCfg_Tx.coderate,
            stRadioCfg_Tx.preambleLen,
            stRadioCfg_Tx.fixLen,
            stRadioCfg_Tx.crcOn,
            stRadioCfg_Tx.freqHopOn,
            stRadioCfg_Tx.hopPeriod,
            stRadioCfg_Tx.iqInverted,
            stRadioCfg_Tx.timeout);
*/
    return len;
}

static const struct file_operations lora_cfg_tx_file_ops ={
    .owner = THIS_MODULE,
    .open = lora_cfg_tx_proc_open,
    .read = seq_read,
    .write = proc_write_lora_cfg_tx,
};

int init_procfs_lora(void)
{
    int rv = 0;

    /* create directory */
    lora_dir = proc_mkdir(MODULE_NAME, NULL);
    if(lora_dir == NULL) {
        rv = -ENOMEM;
        goto out;
    }

    /* create foo and bar files using same callback
     * functions
     */
    lora_cfg_rx_file = proc_create("lora_cfg_rx", 0644, lora_dir,&lora_cfg_rx_file_ops);
    if(lora_cfg_rx_file == NULL) {
        rv = -ENOMEM;
        goto no_foo;
    }
    lora_cfg_tx_file = proc_create("lora_cfg_tx", 0644, lora_dir,&lora_cfg_tx_file_ops);
    if(lora_cfg_rx_file == NULL) {
        rv = -ENOMEM;
        goto no_foo;
    }
    //strcpy(lora_data.name, "lora");

    return 0;


no_foo:
    printk("%s create fail\r\n",MODULE_NAME);
    remove_proc_entry(MODULE_NAME, lora_dir);

out:
    return rv;
}


void cleanup_procfs_lora(void)
{
    remove_proc_entry("lora_cfg_rx", lora_dir);
    remove_proc_entry("lora_cfg_tx", lora_dir);
    //remove_proc_entry("lora_chan", lora_chan_file);
    remove_proc_entry(MODULE_NAME, NULL);
    //remove_proc_entry(MODULE_NAME, NULL);
}
