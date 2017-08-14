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

#define DRV_NAME	"sx1276-spidevs"
#define DRV_DESC	"Sx1276 GPIO-based SPI driver"
#define DRV_VERSION	"0.1"

#define PFX		DRV_NAME ": "


 static const struct of_device_id sx1276_of_match[] = {
         { .compatible = "semtech,sx1276", },
         { }
};
MODULE_DEVICE_TABLE(of, sx1276_of_match);

static int __init sx1276_spidevs_remove(void);
static int __init sx1276_spidevs_probe(void);

static struct spi_driver sx1276_spi_driver = {
         .driver = {
                 .name   = "sx1276",
                 .owner  = THIS_MODULE,
                 .of_match_table = sx1276_of_match,
         },
         .probe          = sx1276_spidevs_probe,
         .remove         = sx1276_spidevs_remove,
};

static irqreturn_t sx1278_1_dio0irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_1_dio1irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_1_dio2irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_1_dio3irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_1_dio4irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_1_dio5irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}

static irqreturn_t sx1278_2_dio0irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_2_dio1irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_2_dio2irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_2_dio3irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_2_dio4irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}
static irqreturn_t sx1278_2_dio5irq_handler(int irq, void *dev_id)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return 0;
}


static int __init sx1276_spidevs_remove(void)
{
	return 0;
}
static int __init sx1276_spidevs_probe(void)
{
	int err;
	unsigned int sx1278_1_dio0irq,sx1278_1_dio1irq,sx1278_1_dio2irq,sx1278_1_dio3irq,sx1278_1_dio4irq,sx1278_1_dio5irq;
	unsigned int sx1278_2_dio0irq,sx1278_2_dio1irq,sx1278_2_dio2irq,sx1278_2_dio3irq,sx1278_2_dio4irq,sx1278_2_dio5irq;
	printk("%s, %d\r\n",__func__,__LINE__);
	printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
	
	gpio_request(SX1278_1_DIO0_PIN, "SX1278_1_DIO0_PIN");
	gpio_direction_input(SX1278_1_DIO0_PIN);
	sx1278_1_dio0irq = gpio_to_irq(SX1278_1_DIO0_PIN);
	request_irq(sx1278_1_dio0irq,sx1278_1_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio0irq",NULL);
	
	gpio_request(SX1278_1_DIO1_PIN, "SX1278_1_DIO1_PIN");
	gpio_direction_input(SX1278_1_DIO1_PIN);
	sx1278_1_dio1irq = gpio_to_irq(SX1278_1_DIO1_PIN);
	request_irq(sx1278_1_dio1irq,sx1278_1_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio1irq",NULL);

	gpio_request(SX1278_1_DIO2_PIN, "SX1278_1_DIO2_PIN");
	gpio_direction_input(SX1278_1_DIO2_PIN);
	sx1278_1_dio2irq = gpio_to_irq(SX1278_1_DIO2_PIN);
	request_irq(sx1278_1_dio2irq,sx1278_1_dio2irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio2irq",NULL);

	gpio_request(SX1278_1_DIO3_PIN, "SX1278_1_DIO3_PIN");
	gpio_direction_input(SX1278_1_DIO3_PIN);
	sx1278_1_dio3irq = gpio_to_irq(SX1278_1_DIO3_PIN);
	request_irq(sx1278_1_dio3irq,sx1278_1_dio3irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio3irq",NULL);

	gpio_request(SX1278_1_DIO4_PIN, "SX1278_1_DIO4_PIN");
	gpio_direction_input(SX1278_1_DIO4_PIN);
	sx1278_1_dio4irq = gpio_to_irq(SX1278_1_DIO4_PIN);
	request_irq(sx1278_1_dio4irq,sx1278_1_dio4irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio4irq",NULL);

	gpio_request(SX1278_1_DIO5_PIN, "SX1278_1_DIO5_PIN");
	gpio_direction_input(SX1278_1_DIO5_PIN);
	sx1278_1_dio5irq = gpio_to_irq(SX1278_1_DIO5_PIN);
	request_irq(sx1278_1_dio5irq,sx1278_1_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio5irq",NULL);


	gpio_request(SX1278_2_DIO0_PIN, "SX1278_2_DIO0_PIN");
	gpio_direction_input(SX1278_2_DIO0_PIN);
	sx1278_1_dio0irq = gpio_to_irq(SX1278_2_DIO0_PIN);
	request_irq(sx1278_2_dio0irq,sx1278_2_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio0irq",NULL);
	
	gpio_request(SX1278_2_DIO1_PIN, "SX1278_2_DIO1_PIN");
	gpio_direction_input(SX1278_2_DIO1_PIN);
	sx1278_2_dio1irq = gpio_to_irq(SX1278_2_DIO1_PIN);
	request_irq(sx1278_2_dio1irq,sx1278_2_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio1irq",NULL);

	gpio_request(SX1278_2_DIO2_PIN, "SX1278_2_DIO2_PIN");
	gpio_direction_input(SX1278_1_DIO2_PIN);
	sx1278_2_dio2irq = gpio_to_irq(SX1278_2_DIO2_PIN);
	request_irq(sx1278_2_dio2irq,sx1278_2_dio2irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio2irq",NULL);

	gpio_request(SX1278_2_DIO3_PIN, "SX1278_2_DIO3_PIN");
	gpio_direction_input(SX1278_2_DIO3_PIN);
	sx1278_2_dio3irq = gpio_to_irq(SX1278_2_DIO3_PIN);
	request_irq(sx1278_2_dio3irq,sx1278_2_dio3irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio3irq",NULL);

	gpio_request(SX1278_2_DIO4_PIN, "SX1278_2_DIO4_PIN");
	gpio_direction_input(SX1278_2_DIO4_PIN);
	sx1278_2_dio4irq = gpio_to_irq(SX1278_2_DIO4_PIN);
	request_irq(sx1278_2_dio4irq,sx1278_2_dio4irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio4irq",NULL);

	gpio_request(SX1278_2_DIO5_PIN, "SX1278_2_DIO5_PIN");
	gpio_direction_input(SX1278_2_DIO5_PIN);
	sx1278_2_dio5irq = gpio_to_irq(SX1278_2_DIO5_PIN);
	request_irq(sx1278_2_dio5irq,sx1278_2_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio5irq",NULL);
	return 0;

err:
	//spi_gpio_custom_cleanup();
	return err;
}
#if 1//def MODULE
static int __init sx1276_spidevs_init(void)
{
	printk("%s, %d\r\n",__func__,__LINE__);
	return sx1276_spidevs_probe();
}
module_init(sx1276_spidevs_init);

static void __init sx1276_spidevs_exit(void)
{
	//spi_gpio_custom_cleanup();
}
module_exit(sx1276_spidevs_exit);
#else
subsys_initcall(sx1276-spidevs_probe);
#endif /* MODULE*/

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Marco Burato <zmaster.adsl@gmail.com>");
MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
