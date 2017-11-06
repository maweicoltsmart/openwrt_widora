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
  
static struct proc_dir_entry *lora_dir, *lora_file;    
  
lora_proc_data_t lora_data;  
    
static int proc_read_lora(char *page, char **start,  
                off_t off, int count,   
                int *eof, void *data)  
{  
    int len;  
    lora_proc_data_t *fb_data = (lora_proc_data_t *)data;  
  
    /* DON'T DO THAT - buffer overruns are bad */  
    len = sprintf(page, "%s = \r\n",   
              fb_data->name);  
  
    return len;  
}  
  
  
static int proc_write_lora(struct file *file, const char *buffer, size_t len, loff_t *off)
{  
    int count;  
  
    if(len > sizeof(lora_proc_data_t))
        count = sizeof(lora_proc_data_t);  
    else  
        count = len;  
  
    if(copy_from_user((void*)&lora_data, buffer, count))  
        return -EFAULT;  
  
    printk("%s: name = %s\r\n",__func__,lora_data.name);  
	if(strcmp(lora_data.name,"RegionCN470RxConfig") == 0)
	{
		SX1276SetRxConfig(lora_data.proc_value.stRxconfig.chip,
			lora_data.proc_value.stRxconfig.modem,
			lora_data.proc_value.stRxconfig.bandwidth,
			lora_data.proc_value.stRxconfig.datarate,
			lora_data.proc_value.stRxconfig.coderate,
			lora_data.proc_value.stRxconfig.bandwidthAfc,
			lora_data.proc_value.stRxconfig.preambleLen,
			lora_data.proc_value.stRxconfig.symbTimeout,
			lora_data.proc_value.stRxconfig.fixLen,
			lora_data.proc_value.stRxconfig.payloadLen,
			lora_data.proc_value.stRxconfig.crcOn,
			lora_data.proc_value.stRxconfig.freqHopOn,
			lora_data.proc_value.stRxconfig.hopPeriod,
			lora_data.proc_value.stRxconfig.iqInverted,
			lora_data.proc_value.stRxconfig.rxContinuous);
	}
	else if(strcmp(lora_data.name,"RegionCN470TxConfig") == 0)
	{
		SX1276SetTxConfig(lora_data.proc_value.stTxconfig.chip,
			lora_data.proc_value.stTxconfig.modem,
			lora_data.proc_value.stTxconfig.power,
			lora_data.proc_value.stTxconfig.fdev,
			lora_data.proc_value.stTxconfig.bandwidth,
			lora_data.proc_value.stTxconfig.datarate,
			lora_data.proc_value.stTxconfig.coderate,
			lora_data.proc_value.stTxconfig.preambleLen,
			lora_data.proc_value.stTxconfig.fixLen,
			lora_data.proc_value.stTxconfig.crcOn,
			lora_data.proc_value.stTxconfig.freqHopOn,
			lora_data.proc_value.stTxconfig.hopPeriod,
			lora_data.proc_value.stTxconfig.iqInverted,
			lora_data.proc_value.stTxconfig.timeout);
	}
	else
	{
		printk("%s: unkwon cmd\r\n",__func__);
	}
    return len;  
}  
  
static const struct file_operations lora_file_ops ={
	.owner = THIS_MODULE,
	.read = proc_read_lora,
	.write = proc_write_lora,
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
    lora_file = proc_create("lora", 0644, lora_dir,&lora_file_ops);  
    if(lora_file == NULL) {  
        rv = -ENOMEM;  
        goto no_foo;  
    }  
  
    strcpy(lora_data.name, "lora");  
         
    return 0;  
  
 
no_foo:  
    remove_proc_entry("jiffies", lora_dir);  
 
out:  
    return rv;  
}  
  
  
void cleanup_procfs_lora(void)  
{
    remove_proc_entry("lora", lora_file);    
	remove_proc_entry(MODULE_NAME, NULL);  
}  
