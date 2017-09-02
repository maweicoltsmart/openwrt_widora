/*!
 * File:
 *  radio_hal.c
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

                /* ======================================= *
                 *              I N C L U D E              *
                 * ======================================= */
#include "typedef.h"
#include "radio_hal.h"
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include "pinmap.h"
#include <linux/gpio.h>
#include <linux/delay.h>


                /* ======================================= *
                 *          D E F I N I T I O N S          *
                 * ======================================= */

                /* ======================================= *
                 *     G L O B A L   V A R I A B L E S     *
                 * ======================================= */

                /* ======================================= *
                 *      L O C A L   F U N C T I O N S      *
                 * ======================================= */

                /* ======================================= *
                 *     P U B L I C   F U N C T I O N S     *
                 * ======================================= */

void radio_hal_AssertShutdown(void)
{
  gpio_set_value(SI4438_SDN,1);
}

void radio_hal_DeassertShutdown(void)
{
  gpio_set_value(SI4438_SDN,0);
}
/*
void radio_hal_ClearNsel(void)
{
    //RF_NSEL = 0;
    ndelay(100);
    gpio_set_value(SI4438_SPI_CS_PIN,0);
    ndelay(30);
}

void radio_hal_SetNsel(void)
{
    //RF_NSEL = 1;
    ndelay(80);
    gpio_set_value(SI4438_SPI_CS_PIN,1);
}*/

/*void set_cs(struct spi_device *spi, bool enable)
{
  if(enable)
  {
    gpio_set_value(SI4438_SPI_CS_PIN,0);
  }
  else
  {
    gpio_set_value(SI4438_SPI_CS_PIN,1);
  }
}*/
/*
bool radio_hal_NirqLevel(void)
{
    //return RF_NIRQ;
    return 0;
}
extern struct spi_device *slave;
void radio_hal_SpiWriteByte(U8 byteToWrite)
{
  spi_write(slave,&byteToWrite,1);
}

U8 radio_hal_SpiReadByte(void)
{
  U8 ret;
  spi_read(slave,&ret,1);
  return ret;
}

void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
  //SpiWriteData(byteCount, pData);
  spi_write(slave,pData,byteCount);
}

void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
  //SpiReadData(byteCount, pData);
  spi_read(slave,pData,byteCount);
}*/

#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
bool radio_hal_Gpio0Level(void)
{
  bool retVal = FALSE;
  //retVal = RF_GPIO0;

  return retVal;
}

bool radio_hal_Gpio1Level(void)
{
  bool retVal = FALSE;
  //retVal = RF_GPIO1;

  return retVal;
}

bool radio_hal_Gpio2Level(void)
{
  bool retVal = FALSE;
  //retVal = RF_GPIO2;

  return retVal;
}

bool radio_hal_Gpio3Level(void)
{
  bool retVal = FALSE;
  //retVal = RF_GPIO3;

  return retVal;
}

#endif
