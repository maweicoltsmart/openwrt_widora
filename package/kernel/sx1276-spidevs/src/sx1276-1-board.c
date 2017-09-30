/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: sx1276_1 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
//#include "board.h"
#include "radio.h"
#include "sx1276-1.h"
#include "sx1276-1-board.h"
#include "typedef.h"
#include <asm/irq.h> //---disable_irq, enable_irq()
#include <linux/interrupt.h> //---request_irq()
#include "pinmap.h"
#include <linux/gpio.h>

/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio_1 =
{
    sx1276_1Init,
    sx1276_1GetStatus,
    sx1276_1SetModem,
    sx1276_1SetChannel,
    sx1276_1IsChannelFree,
    sx1276_1Random,
    sx1276_1SetRxConfig,
    sx1276_1SetTxConfig,
    sx1276_1CheckRfFrequency,
    sx1276_1GetTimeOnAir,
    sx1276_1Send,
    sx1276_1SetSleep,
    sx1276_1SetStby, 
    sx1276_1SetRx,
    sx1276_1StartCad,
    sx1276_1ReadRssi,
    sx1276_1Write,
    sx1276_1Read,
    sx1276_1WriteBuffer,
    sx1276_1ReadBuffer,
    sx1276_1SetMaxPayloadLength
};

unsigned int sx1278_1_dio0irq = 0,sx1278_1_dio1irq = 0,sx1278_1_dio2irq = 0,sx1278_1_dio3irq = 0,sx1278_1_dio4irq = 0,sx1278_1_dio5irq = 0;

extern void sx1276_1OnDio0Irq(unsigned long);
extern void sx1276_1OnDio1Irq(unsigned long);
extern void sx1276_1OnDio2Irq(unsigned long);
extern void sx1276_1OnDio3Irq(unsigned long);
extern void sx1276_1OnDio4Irq(unsigned long);
extern void sx1276_1OnDio5Irq(unsigned long);

DECLARE_TASKLET(sx1276_1OnDio0,sx1276_1OnDio0Irq,0);
DECLARE_TASKLET(sx1276_1OnDio1,sx1276_1OnDio1Irq,0);
DECLARE_TASKLET(sx1276_1OnDio2,sx1276_1OnDio2Irq,0);
DECLARE_TASKLET(sx1276_1OnDio3,sx1276_1OnDio3Irq,0);
DECLARE_TASKLET(sx1276_1OnDio4,sx1276_1OnDio4Irq,0);
DECLARE_TASKLET(sx1276_1OnDio5,sx1276_1OnDio5Irq,0);

static irqreturn_t sx1278_1_dio0irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio0);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio1irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio1);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio2irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio2);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio3irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio3);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio4irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio4);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio5irq_handler(int irq, void *dev_id)
{
    tasklet_schedule(&sx1276_1OnDio5);//调度底半部
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}

/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntSwitchLf;
//Gpio_t AntSwitchHf;
extern struct spi_device *slave[];
void sx1276_1IoInit( void )
{
    int err;
    err = gpio_request(SX1278_1_RST_PIN, "SX1278_1_RST_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_output(SX1278_1_RST_PIN,0);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    sx1276_1.Spi = slave[0];
    err = gpio_request(SX1278_1_DIO0_PIN, "SX1278_1_DIO0_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO0_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_request(SX1278_1_DIO1_PIN, "SX1278_1_DIO1_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO1_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_request(SX1278_1_DIO2_PIN, "SX1278_1_DIO2_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO2_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_request(SX1278_1_DIO3_PIN, "SX1278_1_DIO3_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO3_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_request(SX1278_1_DIO4_PIN, "SX1278_1_DIO4_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO4_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_request(SX1278_1_DIO5_PIN, "SX1278_1_DIO5_PIN");
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
    err = gpio_direction_input(SX1278_1_DIO5_PIN);
    if(err)
    {
        printk("%s,%d\r\n",__func__,__LINE__);
    }
}

void sx1276_1IoIrqInit(void)
{
    int err;
    sx1278_1_dio0irq = gpio_to_irq(SX1278_1_DIO0_PIN);
    err = request_irq(sx1278_1_dio0irq,sx1278_1_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio0irq",NULL);
    //disable_irq(sx1278_1_dio0irq);
    if(err)
    {
        printk( "sx1278_1_dio0irq request failed\r\n");
    }
    sx1278_1_dio1irq = gpio_to_irq(SX1278_1_DIO1_PIN);
    err = request_irq(sx1278_1_dio1irq,sx1278_1_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio1irq",NULL);
    //disable_irq(sx1278_1_dio1irq);
    if(err)
    {
        printk( "sx1278_1_dio1irq request failed\r\n");
    }
    sx1278_1_dio2irq = gpio_to_irq(SX1278_1_DIO2_PIN);
    err = request_irq(sx1278_1_dio2irq,sx1278_1_dio2irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio2irq",NULL);
    //disable_irq(sx1278_1_dio2irq);
    if(err)
    {
        printk( "sx1278_1_dio2irq request failed\r\n");
    }
    sx1278_1_dio3irq = gpio_to_irq(SX1278_1_DIO3_PIN);
    err = request_irq(sx1278_1_dio3irq,sx1278_1_dio3irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio3irq",NULL);
    //disable_irq(sx1278_1_dio3irq);
    if(err)
    {
        printk( "sx1278_1_dio3irq request failed\r\n");
    }
    sx1278_1_dio4irq = gpio_to_irq(SX1278_1_DIO4_PIN);
    err = request_irq(sx1278_1_dio4irq,sx1278_1_dio4irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio4irq",NULL);
    //disable_irq(sx1278_1_dio4irq);
    if(err)
    {
        printk( "sx1278_1_dio4irq request failed\r\n");
    }
    /*sx1278_1_dio5irq = gpio_to_irq(SX1278_1_DIO5_PIN);
    err = request_irq(sx1278_1_dio5irq,sx1278_1_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio5irq",NULL);*/
    //disable_irq(sx1278_1_dio5irq);
    //if(err)
    //{
    //    printk( "sx1278_1_dio5irq request failed\r\n");
    //}
}

void sx1276_1IoDeInit( void )
{
    /*GpioInit( &sx1276_1.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    GpioInit( &sx1276_1.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &sx1276_1.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &sx1276_1.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &sx1276_1.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &sx1276_1.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &sx1276_1.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );*/
}

uint8_t sx1276_1GetPaSelect( uint32_t channel )
{
    if( channel > RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void sx1276_1SetAntSwLowPower( bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;
    
        if( status == false )
        {
            sx1276_1AntSwInit( );
        }
        else
        {
            sx1276_1AntSwDeInit( );
        }
    }
}

void sx1276_1AntSwInit( void )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
}

void sx1276_1AntSwDeInit( void )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
}

void sx1276_1SetAntSw( uint8_t rxTx )
{
    if( sx1276_1.RxTx == rxTx )
    {
        return;
    }

    sx1276_1.RxTx = rxTx;

    if( rxTx != 0 ) // 1: TX, 0: RX
    {
        //GpioWrite( &AntSwitchLf, 0 );
        //GpioWrite( &AntSwitchHf, 1 );
    }
    else
    {
        //GpioWrite( &AntSwitchLf, 1 );
        //GpioWrite( &AntSwitchHf, 0 );
    }
}

bool sx1276_1CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}
