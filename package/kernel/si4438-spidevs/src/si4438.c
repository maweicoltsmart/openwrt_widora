/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic sx1276_1 driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Wael Guibene
*/
#include <linux/string.h>
#include "si4438.h"
#include "si4438-board.h"
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include <linux/spinlock.h>
#include "radio_config.h"
#include "radio.h"
#include "si446x_defs.h"
/*
 * sx1276_1 DIO IRQ callback functions prototype
 */

#define PACKET_SEND_INTERVAL 2000u

/*!
 * \brief DIO 0 IRQ callback
 */
void si4438_OnIRQ( unsigned long );

/*!
 * \brief Tx & Rx timeout timer callback
 */
void sx1276_1OnTimeoutIrq( unsigned long arg );

/*!
 * Tx and Rx timers
 */
static struct timer_list TxTimeoutTimer;
static struct timer_list RxTimeoutTimer;
static struct timer_list RxTimeoutSyncWord;
static struct timeval oldtv;
SEGMENT_VARIABLE(bMain_IT_Status, U8, SEG_XDATA);
SEGMENT_VARIABLE(lPer_MsCnt, U16, SEG_DATA);
extern spinlock_t spi_lock;

void DemoApp_Pollhandler (void);

// Compare the desired custom payload with the incoming payload if variable field is not used
bool  gSampleCode_StringCompare(U8* pbiPacketContent, U8* pbiString, U8 lenght);

// Waiting for a short period of time
//void vSample_Code_Wait(U16 wWaitTime);

// Send "ACK" message  
void vSampleCode_SendAcknowledge(void);

// Send the custom message
bool  gSampleCode_SendVariablePacket(void);
/*!
 * This function is used to compare the content of the received packet to a string.
 *
 * @return  None.
 */
bool gSampleCode_StringCompare(U8* pbiPacketContent, U8* pbiString, U8 lenght)
{
  while ((*pbiPacketContent++ == *pbiString++) && (lenght > 0u))
  {
    if( (--lenght) == 0u )
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*!
 * This function is used to wait for a little time.
 *
 * @return  None.
 */
/*void vSample_Code_Wait(U16 wWaitTime)
{
  SEGMENT_VARIABLE(wDelay, U16 , SEG_DATA) = wWaitTime;

  for (; wDelay--; )
  {
    NOP();
  }
}*/

/*!
 * This function is used to send "ACK"back to the sender.
 *
 * @return  None.
 */
void vSampleCode_SendAcknowledge(void)
{
  // Payload
  customRadioPacket[0u] = 'A';
  customRadioPacket[1u] = 'C';
  customRadioPacket[2u] = 'K';

  // 3 bytes sent to TX FIFO
  vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &customRadioPacket[0], 3u);
}


/*!
 * This function is used to send the custom packet.
 *
 * @return  None.
 */
bool gSampleCode_SendVariablePacket(void)
{
  /*SEGMENT_VARIABLE(boPbPushTrack,  U8, SEG_DATA);
  SEGMENT_VARIABLE(lTemp,         U16, SEG_DATA);
  SEGMENT_VARIABLE(pos,            U8, SEG_DATA);*/
  printk("%s,%d\r\n",__func__,__LINE__);
  vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &customRadioPacket[0], pRadioConfiguration->Radio_PacketLength);
  return FALSE;
}

void DemoApp_Pollhandler()
{
  bMain_IT_Status = bRadio_Check_Tx_RX();

  switch (bMain_IT_Status)
  {
    case SI446X_CMD_GET_INT_STATUS_REP_PACKET_SENT_PEND_BIT:
      printk("send successfully: %c%c%c\r\n",customRadioPacket[0],customRadioPacket[1],customRadioPacket[2]);
      if(gSampleCode_StringCompare(&customRadioPacket[0],"ACK",3) == TRUE)
      {
        // Message "ACK" sent successfully

        // Start RX with radio packet length
        vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, pRadioConfiguration->Radio_PacketLength);
      }
      else
      {
        // Start RX waiting for "ACK" message
        vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 3u);
      }

      break;

    case SI446X_CMD_GET_INT_STATUS_REP_PACKET_RX_PEND_BIT:
      printk("received a packet: %c%c%c \r\n",customRadioPacket[0],customRadioPacket[1],customRadioPacket[2]);
      if (gSampleCode_StringCompare(&customRadioPacket[0], "ACK", 3u) == TRUE)
      {
        // Start RX with radio packet length
        gSampleCode_SendVariablePacket();
        //vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, pRadioConfiguration->Radio_PacketLength);

        break;
      }
      else
      {
        if (gSampleCode_StringCompare((U8 *) &customRadioPacket[0u], pRadioConfiguration->Radio_CustomPayload,
            pRadioConfiguration->Radio_PacketLength) == TRUE )
        {
          // Send ACK back
          vSampleCode_SendAcknowledge();
         }

      }
      break;

    default:
      break;
  } /* switch */
}

void sx1276_1OnTimeoutIrq( unsigned long arg )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
}
extern int event1;
extern wait_queue_head_t wq1;
void si4438_OnIRQ( unsigned long param)
{
	// Demo Application Poll-Handler function
    //DemoApp_Pollhandler();
    event1 = 1;
    wake_up(&wq1);
    //bMain_IT_Status = bRadio_Check_Tx_RX();
}

