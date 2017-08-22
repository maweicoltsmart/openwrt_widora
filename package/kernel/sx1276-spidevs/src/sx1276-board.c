/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: SX1276 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
//#include "board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
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
const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby, 
    SX1276SetRx,
    SX1276StartCad,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength
};

unsigned int sx1278_1_dio0irq,sx1278_1_dio1irq,sx1278_1_dio2irq,sx1278_1_dio3irq,sx1278_1_dio4irq,sx1278_1_dio5irq;
unsigned int sx1278_2_dio0irq,sx1278_2_dio1irq,sx1278_2_dio2irq,sx1278_2_dio3irq,sx1278_2_dio4irq,sx1278_2_dio5irq;

void SX1276OnDio0Irq(void);
void SX1276OnDio1Irq(void);
void SX1276OnDio2Irq(void);
void SX1276OnDio3Irq(void);
void SX1276OnDio4Irq(void);
void SX1276OnDio5Irq(void);

static irqreturn_t sx1278_1_dio0irq_handler(int irq, void *dev_id)
{
    SX1276OnDio0Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio1irq_handler(int irq, void *dev_id)
{
    SX1276OnDio1Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio2irq_handler(int irq, void *dev_id)
{
    SX1276OnDio2Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio3irq_handler(int irq, void *dev_id)
{
    SX1276OnDio3Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio4irq_handler(int irq, void *dev_id)
{
    SX1276OnDio4Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}
static irqreturn_t sx1278_1_dio5irq_handler(int irq, void *dev_id)
{
    SX1276OnDio5Irq();
    printk("%s, %d\r\n",__func__,__LINE__);
    return 0;
}

/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntSwitchLf;
//Gpio_t AntSwitchHf;
extern struct spi_device *slave[];
void SX1276IoInit( void )
{
    gpio_request(SX1278_1_RST_PIN, "SX1278_1_RST_PIN");
    gpio_direction_output(SX1278_1_RST_PIN,0);

    SX1276.Spi = slave[0];
    gpio_request(SX1278_1_DIO0_PIN, "SX1278_1_DIO0_PIN");
    gpio_direction_input(SX1278_1_DIO0_PIN);

    gpio_request(SX1278_1_DIO1_PIN, "SX1278_1_DIO1_PIN");
    gpio_direction_input(SX1278_1_DIO1_PIN);

    gpio_request(SX1278_1_DIO2_PIN, "SX1278_1_DIO2_PIN");
    gpio_direction_input(SX1278_1_DIO2_PIN);

    gpio_request(SX1278_1_DIO3_PIN, "SX1278_1_DIO3_PIN");
    gpio_direction_input(SX1278_1_DIO3_PIN);

    gpio_request(SX1278_1_DIO4_PIN, "SX1278_1_DIO4_PIN");
    gpio_direction_input(SX1278_1_DIO4_PIN);

    gpio_request(SX1278_1_DIO5_PIN, "SX1278_1_DIO5_PIN");
    gpio_direction_input(SX1278_1_DIO5_PIN);
}

void SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    sx1278_1_dio0irq = gpio_to_irq(SX1278_1_DIO0_PIN);
    request_irq(sx1278_1_dio0irq,sx1278_1_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio0irq",NULL);
    //disable_irq(sx1278_1_dio0irq);

    sx1278_1_dio1irq = gpio_to_irq(SX1278_1_DIO1_PIN);
    request_irq(sx1278_1_dio1irq,sx1278_1_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio1irq",NULL);
    //disable_irq(sx1278_1_dio1irq);

    sx1278_1_dio2irq = gpio_to_irq(SX1278_1_DIO2_PIN);
    request_irq(sx1278_1_dio2irq,sx1278_1_dio2irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio2irq",NULL);
    //disable_irq(sx1278_1_dio2irq);

    sx1278_1_dio3irq = gpio_to_irq(SX1278_1_DIO3_PIN);
    request_irq(sx1278_1_dio3irq,sx1278_1_dio3irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio3irq",NULL);
    //disable_irq(sx1278_1_dio3irq);

    sx1278_1_dio4irq = gpio_to_irq(SX1278_1_DIO4_PIN);
    request_irq(sx1278_1_dio4irq,sx1278_1_dio4irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio4irq",NULL);
    //disable_irq(sx1278_1_dio4irq);

    sx1278_1_dio5irq = gpio_to_irq(SX1278_1_DIO5_PIN);
    request_irq(sx1278_1_dio5irq,sx1278_1_dio5irq_handler,IRQF_TRIGGER_RISING,"sx1278_1_dio5irq",NULL);
    //disable_irq(sx1278_1_dio5irq);
}

void SX1276IoDeInit( void )
{
    /*GpioInit( &SX1276.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    GpioInit( &SX1276.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );*/
}

uint8_t SX1276GetPaSelect( uint32_t channel )
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

void SX1276SetAntSwLowPower( bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;
    
        if( status == false )
        {
            SX1276AntSwInit( );
        }
        else
        {
            SX1276AntSwDeInit( );
        }
    }
}

void SX1276AntSwInit( void )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
}

void SX1276AntSwDeInit( void )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
}

void SX1276SetAntSw( uint8_t rxTx )
{
    if( SX1276.RxTx == rxTx )
    {
        return;
    }

    SX1276.RxTx = rxTx;

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

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}
