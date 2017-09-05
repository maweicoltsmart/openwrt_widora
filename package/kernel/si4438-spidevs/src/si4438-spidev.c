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
#include "radio_comm.h"
#include "si4438-board.h"
#include <asm/traps.h>

DEFINE_SPINLOCK(spi_lock);

//#define DRV_NAME	"semtech,sx1278-1"
#define DRV_DESC	"SubGhz Driver"
#define DRV_VERSION	"0.1"

//#define PFX		DRV_NAME ": "

static struct platform_device *devices;
struct spi_device *slave = NULL;
struct spi_master	*master = NULL;

static struct task_struct *radio_routin;

extern unsigned int SI4438_dio0irq,SI4438_dio1irq,SI4438_dio2irq,SI4438_dio3irq,SI4438_dio4irq,SI4438_dio5irq;

static int si4438_spidevs_remove(struct spi_device *spi);
static int si4438_spidevs_probe(struct spi_device *spi);

static void si4438_spidevs_cleanup(void)
{
	int i;
	kthread_stop(radio_routin);

	free_irq(si4438_irq,NULL);
	gpio_free(SI4438_SDN);
	gpio_free(SI4438_NIRQ);
}

static int si4438_spidevs_remove(struct spi_device *spi)
{
	si4438_spidevs_cleanup();
	return 0;
}
static int si4438_spidevs_probe(struct spi_device *spi)
{
	int err;
	slave = spi;
	master = spi->master;
	printk("%s,%d\r\n",__func__,__LINE__);
	printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
	spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    //spi->cs_gpio = SPI_GPIO_NO_CHIPSELECT;
    spi_setup(spi);
	printk("%s, %d\r\n",__func__,__LINE__);
	radio_routin = kthread_create(Radio_routin, NULL, "Radio routin thread");
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
	si4438_spidevs_cleanup();
	return err;
}

static const struct of_device_id subghz_dt_ids[] = {
        { .compatible = "silabs,si4438" },
        {},
};

MODULE_DEVICE_TABLE(of, subghz_dt_ids);

static struct spi_driver si4438_spi_driver = {
        .driver = {
                .name =         "silabs,si4438",
                .owner =        THIS_MODULE,
                .of_match_table = of_match_ptr(subghz_dt_ids),
        },
        .probe =        si4438_spidevs_probe,
        .remove =       si4438_spidevs_remove,

        /* NOTE:  suspend/resume methods are not necessary here.
         * We don't do anything except pass the requests to/from
         * the underlying controller.  The refrigerator handles
         * most issues; the controller driver handles the rest.
         */
};

static int __init subghz_init(void)
{
	int ret;
	printk("%s,%d\r\n",__func__,__LINE__);
	ret = spi_register_driver(&si4438_spi_driver);

	return ret;
}

static void __exit subghz_exit(void)
{
	spi_unregister_driver(&si4438_spi_driver);
}

module_init(subghz_init);
module_exit(subghz_exit);

MODULE_AUTHOR("HorseMa, <wei_technology@163.com>");
MODULE_DESCRIPTION("subghz small gateway use two chips sx1278");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" "silabs,si4438");
