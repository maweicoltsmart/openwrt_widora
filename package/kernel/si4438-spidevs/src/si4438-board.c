/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: si4438_ driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "si4438.h"
#include "si4438-board.h"
#include "typedef.h"
#include <asm/irq.h> //---disable_irq, enable_irq()
#include <linux/interrupt.h> //---request_irq()
#include "pinmap.h"
#include <linux/gpio.h>

extern void si4438_OnIRQ( unsigned long param);
unsigned int si4438_irq = 0;

DECLARE_TASKLET(si4438_OnIRQtl,si4438_OnIRQ,0);

static irqreturn_t si4438_irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&si4438_OnIRQtl);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}

/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntSwitchLf;
//Gpio_t AntSwitchHf;
extern struct spi_device *slave;
void si4438_IoInit( void )
{
    int err;
    err = gpio_request(SI4438_SDN, "SI4438_SDN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_output(SI4438_SDN,0);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    /*err = gpio_request(SI4438_SPI_CS_PIN, "SI4438_SPI_CS_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_output(SI4438_SPI_CS_PIN,1);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }*/
    //si4438_.Spi = slave[0];
    err = gpio_request(SI4438_NIRQ, "SI4438_NIRQ");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SI4438_NIRQ);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
}

void si4438_IoIrqInit(void)
{
    int err;
    si4438_irq = gpio_to_irq(SI4438_NIRQ);
    err = request_irq(si4438_irq,si4438_irq_handler,IRQF_TRIGGER_FALLING,"si4438_irq",NULL);
    //disable_irq(si4438_dio0irq);
    if(err)
    {
        printk( "si4438_irq request failed\r\n");
    }
}

void si4438_IoDeInit( void )
{
    /*GpioInit( &si4438_.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    GpioInit( &si4438_.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &si4438_.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &si4438_.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &si4438_.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &si4438_.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &si4438_.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );*/
}
