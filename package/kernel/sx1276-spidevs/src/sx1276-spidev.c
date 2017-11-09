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
 *  bus0  These four arguments can be arrays of
 *  bus1  1-8 unsigned integers as follows:
 *  bus2
 *  bus3  <id>,<sck>,<mosi>,<miso>,<mode1>,<maxfreq1>,<cs1>,...
 *
 *  where:
 *
 *  <id>  ID to used as device_id for the corresponding bus (required)
 *  <sck> GPIO pin ID to be used for bus SCK (required)
 *  <mosi>  GPIO pin ID to be used for bus MOSI (required*)
 *  <miso>  GPIO pin ID to be used for bus MISO (required*)
 *  <modeX> Mode configuration for slave X in the bus (required)
 *    (see /include/linux/spi/spi.h)
 *  <maxfreqX>  Maximum clock frequency in Hz for slave X in the bus (required)
 *  <csX> GPIO pin ID to be used for slave X CS (required**)
 *
 *  Notes:
 *  * If a signal is not used (for example there is no MISO) you need
 *    to set the GPIO pin ID for that signal to an invalid value.
 *  **  If you only have 1 slave in the bus with no CS, you can omit the
 *    <cs1> param or set it to an invalid GPIO id to disable it. When
 *    you have 2 or more slaves, they must all have a valid CS.
 *
 *  If this driver is built into the kernel, you can use the following kernel
 *  command line parameters, with the same values as the corresponding module
 *  parameters listed above:
 *
 *  sx1276-spidevs.bus0
 *  sx1276-spidevs.bus1
 *  sx1276-spidevs.bus2
 *  sx1276-spidevs.bus3
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
#include "sx1276-cdev.h"
#include "radio-default-param.h"
#include "sx1276-board.h"
#include "proc.h"

DEFINE_SPINLOCK(spi_lock);

//#define DRV_NAME  "semtech,sx1278-1"
#define DRV_DESC  "LoraWAN Driver"
#define DRV_VERSION "0.1"

//#define PFX   DRV_NAME ": "


//static struct task_struct *radio_routin;

extern unsigned int sx1278_1_dio0irq,sx1278_1_dio1irq,sx1278_1_dio2irq,sx1278_1_dio3irq,sx1278_1_dio4irq,sx1278_1_dio5irq;
extern RadioEvents_t RadioEvents;

static int sx1276_spidevs_remove_1(struct spi_device *spi);
static int sx1276_spidevs_probe_1(struct spi_device *spi);
static int sx1276_spidevs_remove_2(struct spi_device *spi);
static int sx1276_spidevs_probe_2(struct spi_device *spi);

static int sx1276_spidevs_remove_1(struct spi_device *spi)
{
    //kthread_stop(radio_routin);
    //unregister_sx1276_cdev();
    //del_timer(&TxTimeoutTimer[0]);
    //del_timer(&RxTimeoutSyncWord[0]);
    //SX1276IoIrqFree(0);
    //SX1276IoFree(0);
    return 0;
}
static int sx1276_spidevs_probe_1(struct spi_device *spi)
{
    int err = 0;
    //int ret;
    //int i;
    SX1276[0].Spi = spi;
    //printk("%s, %d,slave[0] = %d, slave[1] = %d\r\n",__func__,__LINE__,slave[0],slave[1]);
    printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
    spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    err = spi_setup(spi);
    printk("%s, %d\r\n",__func__,__LINE__);
/*
    Radio.Init(0,&RadioEvents);
    Radio.SetChannel(0,470300000);
    Radio.SetTxConfig( 0,MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                               LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                               LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                               true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
    Radio.SetRxConfig( 0,MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    Radio.SetPublicNetwork(0,false);
    Radio.Rx( 0,RX_TIMEOUT_VALUE );
*/
    //register_sx1276_cdev();
    return err;
}

static int sx1276_spidevs_remove_2(struct spi_device *spi)
{
    //kthread_stop(radio_routin);

    //del_timer(&TxTimeoutTimer[1]);
    //del_timer(&RxTimeoutSyncWord[1]);
    //SX1276IoIrqFree(1);
    //SX1276IoFree(1);
    unregister_sx1276_cdev();
    cleanup_procfs_lora();
    return 0;
}
static int sx1276_spidevs_probe_2(struct spi_device *spi)
{
    int err = 0;
    //int ret;
    //int i;
    SX1276[1].Spi = spi;
    //printk("%s, %d,slave[0] = %d, slave[1] = %d\r\n",__func__,__LINE__,slave[0],slave[1]);
    printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
    spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    err = spi_setup(spi);
    printk("%s, %d\r\n",__func__,__LINE__);
/*
    Radio.Init(1,&RadioEvents);
    Radio.SetChannel(1,470500000);
    Radio.SetTxConfig( 1,MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                               LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                               LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                               true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
    Radio.SetRxConfig( 1,MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    Radio.SetPublicNetwork(1,false);
    Radio.Rx( 1,RX_TIMEOUT_VALUE );
    */
    register_sx1276_cdev();
    init_procfs_lora();
    return err;
}

static const struct of_device_id lorawan_dt_ids_1[] = {
    { .compatible = "semtech,sx1278-1" },
    {},
};

MODULE_DEVICE_TABLE(of, lorawan_dt_ids_1);

static struct spi_driver lorawan_spi_driver_1 = {
        .driver = {
        .name =         "semtech,sx1278-1",
        .owner =        THIS_MODULE,
        .of_match_table = of_match_ptr(lorawan_dt_ids_1),
    },
    .probe =        sx1276_spidevs_probe_1,
    .remove =       sx1276_spidevs_remove_1,

    /* NOTE:  suspend/resume methods are not necessary here.
    * We don't do anything except pass the requests to/from
    * the underlying controller.  The refrigerator handles
    * most issues; the controller driver handles the rest.
    */
};

static const struct of_device_id lorawan_dt_ids_2[] = {
    { .compatible = "semtech,sx1278-2" },
    {},
};

MODULE_DEVICE_TABLE(of, lorawan_dt_ids_2);

static struct spi_driver lorawan_spi_driver_2 = {
        .driver = {
        .name =         "semtech,sx1278-2",
        .owner =        THIS_MODULE,
        .of_match_table = of_match_ptr(lorawan_dt_ids_2),
    },
    .probe =        sx1276_spidevs_probe_2,
    .remove =       sx1276_spidevs_remove_2,

    /* NOTE:  suspend/resume methods are not necessary here.
    * We don't do anything except pass the requests to/from
    * the underlying controller.  The refrigerator handles
    * most issues; the controller driver handles the rest.
    */
};
static int __init lorawan_init(void)
{
    int ret;
    ret = spi_register_driver(&lorawan_spi_driver_1);
    ret = spi_register_driver(&lorawan_spi_driver_2);
    return ret;
}

static void __exit lorawan_exit(void)
{
    spi_unregister_driver(&lorawan_spi_driver_1);
    spi_unregister_driver(&lorawan_spi_driver_2);
}

module_init(lorawan_init);
module_exit(lorawan_exit);

MODULE_AUTHOR("HorseMa, <wei_technology@163.com>");
MODULE_DESCRIPTION("LoraWAN small gateway use two chips sx1278");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" "semtech,sx1278");
