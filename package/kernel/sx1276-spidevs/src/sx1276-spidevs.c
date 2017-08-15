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

#define DRV_NAME	"sx1276-spidevs"
#define DRV_DESC	"Sx1276 GPIO-based SPI driver"
#define DRV_VERSION	"0.1"

#define PFX		DRV_NAME ": "

#define BUS_PARAM_ID		0
#define BUS_PARAM_SCK		1
#define BUS_PARAM_MOSI		2
#define BUS_PARAM_MISO		3
#define BUS_PARAM_MODE1		4
#define BUS_PARAM_MAXFREQ1	5
#define BUS_PARAM_CS1		6

#define BUS_SLAVE_COUNT_MAX	2
#define BUS_PARAM_REQUIRED	6
#define BUS_PARAM_PER_SLAVE	3
#define BUS_PARAM_COUNT		(4+BUS_PARAM_PER_SLAVE*BUS_SLAVE_COUNT_MAX)

static unsigned int bus_nump[1] = {10};
static struct platform_device *devices;
struct spi_device *slave[2];

#define BUS_PARM_DESC \
	" config -> id,sck,mosi,miso,mode1,maxfreq1[,cs1,mode2,maxfreq2,cs2,...]"

static unsigned int sx1276_spi_gpio_params[BUS_PARAM_COUNT] = {
		0,
		SX1278_1_SPI_CLK_PIN,
		SX1278_1_SPI_MOSI_PIN,
		SX1278_1_SPI_MISO_PIN,
		SPI_MODE_0,
		1000000,
		SX1278_1_SPI_CS_PIN,
		SPI_MODE_0,
		1000000,
		SX1278_2_SPI_CS_PIN
};

 /*static const struct of_device_id sx1276_of_match[] = {
         { .compatible = "semtech,sx1276", },
         { }
};
MODULE_DEVICE_TABLE(of, sx1276_of_match);
*/
static int __init sx1276_spidevs_remove(void);
static int __init sx1276_spidevs_probe(void);

/*static struct spi_driver sx1276_spi_driver = {
         .driver = {
                 .name   = "sx1276",
                 .owner  = THIS_MODULE,
                 .of_match_table = sx1276_of_match,
         },
         .probe          = sx1276_spidevs_probe,
         .remove         = sx1276_spidevs_remove,
};

static struct spi_gpio_platform_data sx1276_spi_gpio_data = {
		.sck    = SX1278_1_SPI_CLK_PIN,    //GPIO_SPI_SCK,
		.mosi    = SX1278_1_SPI_MOSI_PIN,     //GPIO_SPI_MOSI,
		.miso    = SX1278_1_SPI_MISO_PIN,    //GPIO_SPI_MISO,
		.num_chipselect = 2,
};

struct platform_device sx1276_spi_gpio_device = {
		.name   = "spi_gpio",
		.id     = 0, 
		.dev    = {
		.platform_data = &sx1276_spi_gpio_data,
	},
};
*/

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

static void spi_gpio_custom_cleanup(void)
{
	int i;

	if (devices)
		platform_device_unregister(devices);
}

static int spi_gpio_custom_get_slave_mode(unsigned int id,
					  unsigned int *params,
					  int slave_index)
{
	int param_index;

	param_index = BUS_PARAM_MODE1+slave_index*BUS_PARAM_PER_SLAVE;
	if (param_index >= bus_nump[id])
		return -1;

	return params[param_index];
}
static int spi_gpio_custom_get_slave_maxfreq(unsigned int id,
					     unsigned int *params,
					     int slave_index)
{
	int param_index;

	param_index = BUS_PARAM_MAXFREQ1+slave_index*BUS_PARAM_PER_SLAVE;
	if (param_index >= bus_nump[id])
		return -1;

	return params[param_index];
}
static int spi_gpio_custom_get_slave_cs(unsigned int id,
					unsigned int *params,
					int slave_index)
{
	int param_index;

	param_index = BUS_PARAM_CS1+slave_index*BUS_PARAM_PER_SLAVE;
	if (param_index >= bus_nump[id])
		return -1;
	if (!gpio_is_valid(params[param_index]))
		return -1;

	return params[param_index];
}

static int spi_gpio_custom_check_params(unsigned int id, unsigned int *params)
{
	int i;
	struct spi_master *master;

	if (bus_nump[id] < BUS_PARAM_REQUIRED) {
		printk(KERN_ERR PFX "not enough values for parameter bus%d\n",
		       id);
		return -EINVAL;
	}

	if (bus_nump[id] > (1+BUS_PARAM_CS1)) {
		/* more than 1 device: check CS GPIOs */
		for (i = 0; i < BUS_SLAVE_COUNT_MAX; i++) {
			/* no more slaves? */
			if (spi_gpio_custom_get_slave_mode(id, params, i) < 0)
				break;

			if (spi_gpio_custom_get_slave_cs(id, params, i) < 0) {
				printk(KERN_ERR PFX "invalid/missing CS gpio for slave %d on bus %d\n",
				       i, params[BUS_PARAM_ID]);
				return -EINVAL;
			}
		}
	}

	if (!gpio_is_valid(params[BUS_PARAM_SCK])) {
		printk(KERN_ERR PFX "invalid SCK gpio for bus %d\n",
		       params[BUS_PARAM_ID]);
		return -EINVAL;
	}

	master = spi_busnum_to_master(params[BUS_PARAM_ID]);
	if (master) {
		spi_master_put(master);
		printk(KERN_ERR PFX "bus %d already exists\n",
		       params[BUS_PARAM_ID]);
		return -EEXIST;
	}

	return 0;
}

static int __init spi_gpio_custom_add_one(unsigned int id, unsigned int *params)
{
	struct platform_device *pdev;
	struct spi_gpio_platform_data pdata;
	int i;
	int num_cs;
	int err;
	struct spi_master *master;
	//struct spi_device *slave;
	struct spi_board_info slave_info;
	int mode, maxfreq, cs;


	/*if (!bus_nump[id])
		return 0;*/

	err = spi_gpio_custom_check_params(id, params);
	if (err)
		goto err;

	/* Create BUS device node */

	pdev = platform_device_alloc("spi_gpio", params[BUS_PARAM_ID]);
	if (!pdev) {
		err = -ENOMEM;
		goto err;
	}

	num_cs = 0;
	for (i = 0; i < BUS_SLAVE_COUNT_MAX; i++) {
		/* no more slaves? */
		if (spi_gpio_custom_get_slave_mode(id, params, i) < 0)
			break;

		if (spi_gpio_custom_get_slave_cs(id, params, i) >= 0)
			num_cs++;
	}
	if (num_cs == 0) {
		/*
		 * Even if no CS is used, spi modules expect
		 * at least 1 (unused)
		 */
		num_cs = 1;
	}

	pdata.sck = params[BUS_PARAM_SCK];
	pdata.mosi = gpio_is_valid(params[BUS_PARAM_MOSI])
		? params[BUS_PARAM_MOSI]
		: SPI_GPIO_NO_MOSI;
	pdata.miso = gpio_is_valid(params[BUS_PARAM_MISO])
		? params[BUS_PARAM_MISO]
		: SPI_GPIO_NO_MISO;
	pdata.num_chipselect = num_cs;

	err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
	if (err) {
		platform_device_put(pdev);
		goto err;
	}

	err = platform_device_add(pdev);
	if (err) {
		printk(KERN_ERR PFX "platform_device_add failed with return code %d\n",
		       err);
		platform_device_put(pdev);
		goto err;
	}

	/* Register SLAVE devices */

	for (i = 0; i < BUS_SLAVE_COUNT_MAX; i++) {
		mode = spi_gpio_custom_get_slave_mode(id, params, i);
		maxfreq = spi_gpio_custom_get_slave_maxfreq(id, params, i);
		cs = spi_gpio_custom_get_slave_cs(id, params, i);

		/* no more slaves? */
		if (mode < 0)
			break;

		memset(&slave_info, 0, sizeof(slave_info));
		strcpy(slave_info.modalias, "spidev");
		slave_info.controller_data = (void *)((cs >= 0)
			? cs
			: SPI_GPIO_NO_CHIPSELECT);
		slave_info.max_speed_hz = maxfreq;
		slave_info.bus_num = params[BUS_PARAM_ID];
		slave_info.chip_select = i;
		slave_info.mode = mode;

		master = spi_busnum_to_master(params[BUS_PARAM_ID]);
		if (!master) {
			printk(KERN_ERR PFX "unable to get master for bus %d\n",
			       params[BUS_PARAM_ID]);
			err = -EINVAL;
			goto err_unregister;
		}
		slave[i] = spi_new_device(master, &slave_info);
		spi_master_put(master);
		if (!slave[i]) {
			printk(KERN_ERR PFX "unable to create slave %d for bus %d\n",
			       i, params[BUS_PARAM_ID]);
			/* Will most likely fail due to unsupported mode bits */
			err = -EINVAL;
			goto err_unregister;
		}
	}

	devices = pdev;

	return 0;

err_unregister:
	platform_device_unregister(pdev);
err:
	return err;
}

static int __init sx1276_spidevs_remove(void)
{
	return 0;
}
static int __init sx1276_spidevs_probe(void)
{
	int err;
	int chipversion;

	unsigned int sx1278_1_dio0irq,sx1278_1_dio1irq,sx1278_1_dio2irq,sx1278_1_dio3irq,sx1278_1_dio4irq,sx1278_1_dio5irq;
	unsigned int sx1278_2_dio0irq,sx1278_2_dio1irq,sx1278_2_dio2irq,sx1278_2_dio3irq,sx1278_2_dio4irq,sx1278_2_dio5irq;
	printk("%s, %d\r\n",__func__,__LINE__);
	printk(KERN_INFO DRV_DESC " version " DRV_VERSION "\n");
	
	//platform_device_register(&sx1276_spi_gpio_device);
	err = spi_gpio_custom_add_one(0, sx1276_spi_gpio_params);
	if (err)
		goto err;
	spi_setup(slave[0]);
	spi_setup(slave[1]);

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

	/*gpio_request(SX1278_1_DIO5_PIN, "SX1278_1_DIO5_PIN");
	gpio_direction_input(SX1278_1_DIO5_PIN);
	sx1278_1_dio5irq = gpio_to_irq(SX1278_1_DIO5_PIN);
	request_irq(sx1278_1_dio5irq,sx1278_1_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio5irq",NULL);
*/

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

	/*gpio_request(SX1278_2_DIO5_PIN, "SX1278_2_DIO5_PIN");
	gpio_direction_input(SX1278_2_DIO5_PIN);
	sx1278_2_dio5irq = gpio_to_irq(SX1278_2_DIO5_PIN);
	request_irq(sx1278_2_dio5irq,sx1278_2_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio5irq",NULL);
*/

	if(devices == NULL)
	{
		printk("devices is NULL\r\n");
		return 0;
	}
	/*err = gpio_request(SX1278_1_SPI_CS_PIN, "SX1278_1_SPI_CS_PIN");
	printk("%s, %d\r\n",__func__,__LINE__);
	if (err)
		goto err;
	err = gpio_request(SX1278_2_SPI_CS_PIN, "SX1278_2_SPI_CS_PIN");
	printk("%s, %d\r\n",__func__,__LINE__);
	if (err)
		goto err;*/
	err = gpio_request(SX1278_1_RST_PIN, "SX1278_1_RST_PIN");
	printk("%s, %d\r\n",__func__,__LINE__);
	if (err)
		goto err;
	err = gpio_request(SX1278_2_RST_PIN, "SX1278_2_RST_PIN");
	printk("%s, %d\r\n",__func__,__LINE__);
	if (err)
		goto err;

	//gpio_direction_output(SX1278_1_SPI_CS_PIN,1);
	//gpio_direction_output(SX1278_2_SPI_CS_PIN,1);
	udelay(10000);
	gpio_direction_output(SX1278_1_RST_PIN,0);
	gpio_direction_output(SX1278_2_RST_PIN,0);
	udelay(10000);
	gpio_set_value(SX1278_1_RST_PIN,1);
	gpio_set_value(SX1278_2_RST_PIN,1);
	udelay(100000);
	printk("now read chip version through spi\r\n");

	//gpio_set_value(SX1278_1_SPI_CS_PIN,0);
	udelay(1000);
	spi_w8r8(slave[0],0x42 & 0x7f);
	chipversion = spi_w8r8(slave[0],0x00);
	//gpio_set_value(SX1278_1_SPI_CS_PIN,1);
	printk("%s:chipversion is 0x%02x\r\n",__func__,chipversion);

	udelay(10000);

	//gpio_set_value(SX1278_2_SPI_CS_PIN,0);
	udelay(1000);
	spi_w8r8(slave[1],0x42 & 0x7f);
	chipversion = spi_w8r8(slave[1],0x00);
	//gpio_set_value(SX1278_2_SPI_CS_PIN,1);
	printk("%s:chipversion is 0x%02x\r\n",__func__,chipversion);

	return 0;

err:
	spi_gpio_custom_cleanup();
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
	spi_gpio_custom_cleanup();
}
module_exit(sx1276_spidevs_exit);
#else
subsys_initcall(sx1276-spidevs_probe);
#endif /* MODULE*/

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Marco Burato <zmaster.adsl@gmail.com>");
MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
