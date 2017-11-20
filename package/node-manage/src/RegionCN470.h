/*!
 * \file      RegionCN470.h
 *
 * \brief     Region definition for CN470
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \defgroup  REGIONCN470 Region CN470
 *            Implementation according to LoRaWAN Specification v1.0.2.
 * \{
 */
#ifndef __REGION_CN470_H__
#define __REGION_CN470_H__
#include "typedef.h"

/*!
 * LoRaMac maximum number of channels
 */
#define CN470_MAX_NB_CHANNELS                        96

/*!
 * Minimal datarate that can be used by the node
 */
#define CN470_TX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define CN470_TX_MAX_DATARATE                       DR_5

/*!
 * Minimal datarate that can be used by the node
 */
#define CN470_RX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define CN470_RX_MAX_DATARATE                       DR_5

/*!
 * Default datarate used by the node
 */
#define CN470_DEFAULT_DATARATE                      DR_0

/*!
 * Minimal Rx1 receive datarate offset
 */
#define CN470_MIN_RX1_DR_OFFSET                     0

/*!
 * Maximal Rx1 receive datarate offset
 */
#define CN470_MAX_RX1_DR_OFFSET                     3

/*!
 * Default Rx1 receive datarate offset
 */
#define CN470_DEFAULT_RX1_DR_OFFSET                 0

/*!
 * Minimal Tx output power that can be used by the node
 */
#define CN470_MIN_TX_POWER                        TX_POWER_7

/*!
 * Maximal Tx output power that can be used by the node
 */
#define CN470_MAX_TX_POWER                        TX_POWER_0

/*!
 * Default Tx output power used by the node
 */
#define CN470_DEFAULT_TX_POWER                    TX_POWER_0

/*!
 * Default Max EIRP
 */
#define CN470_DEFAULT_MAX_EIRP                      22.15f

/*!
 * Default antenna gain
 */
#define CN470_DEFAULT_ANTENNA_GAIN                  2.15f

/*!
 * ADR Ack limit
 */
#define CN470_ADR_ACK_LIMIT                         64

/*!
 * ADR Ack delay
 */
#define CN470_ADR_ACK_DELAY                         32

/*!
 * Enabled or disabled the duty cycle
 */
#define CN470_DUTY_CYCLE_ENABLED                    0

/*!
 * Maximum RX window duration
 */
#define CN470_MAX_RX_WINDOW                         3000

/*!
 * Receive delay 1
 */
#define CN470_RECEIVE_DELAY1                        1000

/*!
 * Receive delay 2
 */
#define CN470_RECEIVE_DELAY2                        2000

/*!
 * Join accept delay 1
 */
#define CN470_JOIN_ACCEPT_DELAY1                    5000

/*!
 * Join accept delay 2
 */
#define CN470_JOIN_ACCEPT_DELAY2                    6000

/*!
 * Maximum frame counter gap
 */
#define CN470_MAX_FCNT_GAP                          16384

/*!
 * Ack timeout
 */
#define CN470_ACKTIMEOUT                            2000

/*!
 * Random ack timeout limits
 */
#define CN470_ACK_TIMEOUT_RND                       1000

/*!
 * Second reception window channel frequency definition.
 */
#define CN470_RX_WND_2_FREQ                         505300000

/*!
 * Second reception window channel datarate definition.
 */
#define CN470_RX_WND_2_DR                           DR_0

/*!
 * LoRaMac maximum number of bands
 */
#define CN470_MAX_NB_BANDS                           1

/*!
 * Band 0 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define CN470_BAND0                                 { 1, CN470_MAX_TX_POWER, 0,  0 } //  100.0 %

/*!
 * Defines the first channel for RX window 1 for CN470 band
 */
#define CN470_FIRST_RX1_CHANNEL                     ( (uint32_t) 500300000 )

/*!
 * Defines the last channel for RX window 1 for CN470 band
 */
#define CN470_LAST_RX1_CHANNEL                      ( (uint32_t) 509700000 )

/*!
 * Defines the step width of the channels for RX window 1
 */
#define CN470_STEPWIDTH_RX1_CHANNEL                 ( (uint32_t) 200000 )

/*!
 * Data rates table definition
 */
static const uint8_t DataratesCN470[]  = { 12, 11, 10,  9,  8,  7 };

/*!
 * Bandwidths table definition in Hz
 */
static const uint32_t BandwidthsCN470[] = { 125000, 125000, 125000, 125000, 125000, 125000 };

/*!
 * Maximum payload with respect to the datarate index. Cannot operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateCN470[] = { 51, 51, 51, 115, 222, 222 };

/*!
 * Maximum payload with respect to the datarate index. Can operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateRepeaterCN470[] = { 51, 51, 51, 115, 222, 222 };


#endif // __REGION_CN470_H__
