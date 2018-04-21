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
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/workqueue.h>

#include "utilities.h"
#include "Radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include "pinmap.h"
#include "NodeDatabase.h"

/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;
extern struct spi_device *slave;

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
    SX1276SetTxContinuousWave,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength,
    SX1276SetPublicNetwork
};

unsigned int sx1278_1_dio0irq,sx1278_1_dio1irq;
unsigned int sx1278_2_dio0irq,sx1278_2_dio1irq;
unsigned int sx1278_3_dio0irq,sx1278_3_dio1irq;
unsigned int sx1278_4_dio0irq,sx1278_4_dio1irq;

extern void SX1276OnDio0Irq(struct work_struct *p_work);
extern void SX1276OnDio1Irq(struct work_struct *p_work);
extern struct workqueue_struct *RadioWorkQueue;

static st_MyWork stMyWorkChip1Dio0,stMyWorkChip1Dio1;
static st_MyWork stMyWorkChip2Dio0,stMyWorkChip2Dio1;
static st_MyWork stMyWorkChip3Dio0,stMyWorkChip3Dio1;
static st_MyWork stMyWorkChip4Dio0,stMyWorkChip4Dio1;

static irqreturn_t sx1278_1_dio0irq_handler(int irq, void *dev_id)
{
    stMyWorkChip1Dio0.param = 0;
    INIT_WORK(&(stMyWorkChip1Dio0.save), SX1276OnDio0Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip1Dio0.save));
    return 0;
}
static irqreturn_t sx1278_1_dio1irq_handler(int irq, void *dev_id)
{
    stMyWorkChip1Dio1.param = 0;
    INIT_WORK(&(stMyWorkChip1Dio1.save), SX1276OnDio1Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip1Dio1.save));
    return 0;
}

static irqreturn_t sx1278_2_dio0irq_handler(int irq, void *dev_id)
{
    stMyWorkChip2Dio0.param = 1;
    INIT_WORK(&(stMyWorkChip2Dio0.save), SX1276OnDio0Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip2Dio0.save));
    return 0;
}
static irqreturn_t sx1278_2_dio1irq_handler(int irq, void *dev_id)
{
    stMyWorkChip2Dio1.param = 1;
    INIT_WORK(&(stMyWorkChip2Dio1.save), SX1276OnDio1Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip2Dio1.save));
    return 0;
}

static irqreturn_t sx1278_3_dio0irq_handler(int irq, void *dev_id)
{
    stMyWorkChip3Dio0.param = 2;
    INIT_WORK(&(stMyWorkChip3Dio0.save), SX1276OnDio0Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip3Dio0.save));
    return 0;
}
static irqreturn_t sx1278_3_dio1irq_handler(int irq, void *dev_id)
{
    stMyWorkChip3Dio1.param = 2;
    INIT_WORK(&(stMyWorkChip3Dio1.save), SX1276OnDio1Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip3Dio1.save));
    return 0;
}

static irqreturn_t sx1278_4_dio0irq_handler(int irq, void *dev_id)
{
    stMyWorkChip4Dio0.param = 3;
    INIT_WORK(&(stMyWorkChip4Dio0.save), SX1276OnDio0Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip4Dio0.save));
    return 0;
}
static irqreturn_t sx1278_4_dio1irq_handler(int irq, void *dev_id)
{
    stMyWorkChip4Dio1.param = 3;
    INIT_WORK(&(stMyWorkChip4Dio1.save), SX1276OnDio1Irq);
    queue_work(RadioWorkQueue, &(stMyWorkChip4Dio1.save));
    return 0;
}


/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntSwitchLf;
//Gpio_t AntSwitchHf;

void SX1276IoInit( int chip )
{
    int err;
    switch(chip)
    {
        case 0:
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
            //SX1276.Spi = slave;
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
            err = gpio_direction_output(SX1278_1_DIO2_PIN,0);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            break;
        case 1:
            err = gpio_request(SX1278_2_RST_PIN, "SX1278_2_RST_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_output(SX1278_2_RST_PIN,0);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            //SX1276.Spi = slave;
            err = gpio_request(SX1278_2_DIO0_PIN, "SX1278_2_DIO0_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_input(SX1278_2_DIO0_PIN);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_request(SX1278_2_DIO1_PIN, "SX1278_2_DIO1_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_input(SX1278_2_DIO1_PIN);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_request(SX1278_2_DIO2_PIN, "SX1278_2_DIO2_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_output(SX1278_2_DIO2_PIN,0);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            break;
        case 2:
            err = gpio_request(SX1278_3_RST_PIN, "SX1278_3_RST_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_output(SX1278_3_RST_PIN,0);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            //SX1276.Spi = slave;
            err = gpio_request(SX1278_3_DIO0_PIN, "SX1278_3_DIO0_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_input(SX1278_3_DIO0_PIN);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_request(SX1278_3_DIO1_PIN, "SX1278_3_DIO1_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_input(SX1278_3_DIO1_PIN);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_request(SX1278_3_DIO2_PIN, "SX1278_3_DIO2_PIN");
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            err = gpio_direction_output(SX1278_3_DIO2_PIN,0);
            if(err)
            {
                printk("%s,%d\r\n",__func__,__LINE__);
            }
            break;
        default:
            break;
    }
}

void SX1276IoIrqInit( int chip )
{
    int err;
    switch(chip)
    {
        case 0:
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
            break;
        case 1:
            sx1278_2_dio0irq = gpio_to_irq(SX1278_2_DIO0_PIN);
            err = request_irq(sx1278_2_dio0irq,sx1278_2_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio0irq",NULL);
            //disable_irq(sx1278_2_dio0irq);
            if(err)
            {
                printk( "sx1278_2_dio0irq request failed\r\n");
            }
            sx1278_2_dio1irq = gpio_to_irq(SX1278_2_DIO1_PIN);
            err = request_irq(sx1278_2_dio1irq,sx1278_2_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_2_dio1irq",NULL);
            //disable_irq(sx1278_2_dio1irq);
            if(err)
            {
                printk( "sx1278_2_dio1irq request failed\r\n");
            }
            break;
        case 2:
			sx1278_3_dio0irq = gpio_to_irq(SX1278_3_DIO0_PIN);
            err = request_irq(sx1278_3_dio0irq,sx1278_3_dio0irq_handler,IRQF_TRIGGER_RISING,"sx1278_3_dio0irq",NULL);
            //disable_irq(sx1278_3_dio0irq);
            if(err)
            {
                printk( "sx1278_3_dio0irq request failed\r\n");
            }
            sx1278_3_dio1irq = gpio_to_irq(SX1278_3_DIO1_PIN);
            err = request_irq(sx1278_3_dio1irq,sx1278_3_dio1irq_handler,IRQF_TRIGGER_RISING,"sx1278_3_dio1irq",NULL);
            //disable_irq(sx1278_3_dio1irq);
            if(err)
            {
                printk( "sx1278_3_dio1irq request failed\r\n");
            }
            break;
        default:
            break;
    }
}
void SX1276IoDeInit( void )
{

}

void SX1276SetRfTxPower( int chip ,int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = SX1276Read( chip,REG_PACONFIG );
    paDac = SX1276Read( chip,REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1276GetPaSelect( chip,SX1276[chip].Settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1276Write( chip,REG_PACONFIG, paConfig );
    SX1276Write( chip,REG_PADAC, paDac );
}

uint8_t SX1276GetPaSelect( int chip,uint32_t channel )
{
    if( channel < RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void SX1276SetAntSwLowPower( int chip,bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;

        if( status == false )
        {
            SX1276AntSwInit( chip);
        }
        else
        {
            SX1276AntSwDeInit( chip);
        }
    }
}

void SX1276AntSwInit( int chip )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
}

void SX1276AntSwDeInit( int chip )
{
    //GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
    //GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
}

void SX1276SetAntSw( int chip,uint8_t opMode )
{
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:
	switch(chip)
	{
		case 0:
		gpio_set_value(SX1278_1_DIO2_PIN,0);
		break;
		case 1:
		gpio_set_value(SX1278_2_DIO2_PIN,0);
		break;
		case 2:
		gpio_set_value(SX1278_3_DIO2_PIN,0);
		break;
		default:
		break;
	}
        //GpioWrite( &AntSwitchLf, 0 );
        //GpioWrite( &AntSwitchHf, 1 );
        break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
    default:
	switch(chip)
        {
                case 0:
                gpio_set_value(SX1278_1_DIO2_PIN,1);
                break;
                case 1:
                gpio_set_value(SX1278_2_DIO2_PIN,1);
                break;
                case 2:
                gpio_set_value(SX1278_3_DIO2_PIN,1);
                break;
                default:
                break;
        }
        //GpioWrite( &AntSwitchLf, 1 );
        //GpioWrite( &AntSwitchHf, 0 );
        break;
    }
}

bool SX1276CheckRfFrequency( int chip,uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

void SX1276IoFree(int chip)
{
    switch(chip)
    {
        case 0:
            gpio_free(SX1278_1_RST_PIN);
            gpio_free(SX1278_1_DIO0_PIN);
            gpio_free(SX1278_1_DIO1_PIN);
            gpio_free(SX1278_1_DIO2_PIN);
            break;
        case 1:
            gpio_free(SX1278_2_RST_PIN);
            gpio_free(SX1278_2_DIO0_PIN);
            gpio_free(SX1278_2_DIO1_PIN);
            gpio_free(SX1278_2_DIO2_PIN);
            break;
        case 2:
			gpio_free(SX1278_3_RST_PIN);
            gpio_free(SX1278_3_DIO0_PIN);
            gpio_free(SX1278_3_DIO1_PIN);
            gpio_free(SX1278_3_DIO2_PIN);
            break;
        default:
            break;
    }
}

void SX1276IoIrqFree(int chip)
{
    switch(chip)
    {
        case 0:
            free_irq(sx1278_1_dio0irq,NULL);
            free_irq(sx1278_1_dio1irq,NULL);
            break;
        case 1:
            free_irq(sx1278_2_dio0irq,NULL);
            free_irq(sx1278_2_dio1irq,NULL);
            break;
        case 2:
			free_irq(sx1278_3_dio0irq,NULL);
            free_irq(sx1278_3_dio1irq,NULL);
            break;
        default:
            break;
    }
}
