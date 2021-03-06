/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic SX1276 driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Wael Guibene
*/
#include <linux/string.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <asm/ptrace.h>
#include <asm/irq.h>
#include <linux/interrupt.h>

#include "Radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include "utilities.h"
#include "pinmap.h"
#include "NodeDatabase.h"

/*
 * Local types definition
 */

/*!
 * Radio registers definition
 */
typedef struct
{
    RadioModems_t Modem;
    uint8_t       Addr;
    uint8_t       Value;
}RadioRegisters_t;

/*!
 * FSK bandwidth definition
 */
typedef struct
{
    uint32_t bandwidth;
    uint8_t  RegValue;
}FskBandwidth_t;


/*
 * Private functions prototypes
 */

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( int chip );

/*!
 * \brief Resets the SX1276
 */
void SX1276Reset( int chip );

/*!
 * \brief Sets the SX1276 in transmission mode for the given time
 * \param [IN] timeout Transmission timeout [ms] [0: continuous, others timeout]
 */
void SX1276SetTx( int chip,uint32_t timeout );

/*!
 * \brief Writes the buffer contents to the SX1276 FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void SX1276WriteFifo( int chip,uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads the contents of the SX1276 FIFO
 *
 * \param [OUT] buffer Buffer where to copy the FIFO read data.
 * \param [IN] size Number of bytes to be read from the FIFO
 */
void SX1276ReadFifo( int chip,uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the SX1276 operating mode
 *
 * \param [IN] opMode New operating mode
 */
void SX1276SetOpMode( int chip,uint8_t opMode );

/*
 * SX1276 DIO IRQ callback functions prototype
 */

/*!
 * \brief DIO 0 IRQ callback
 */
void SX1276OnDio0Irq(struct work_struct *p_work);

/*!
 * \brief DIO 1 IRQ callback
 */
void SX1276OnDio1Irq(struct work_struct *p_work);

/*!
 * \brief DIO 2 IRQ callback
 */
void SX1276OnDio2Irq( int chip );

/*!
 * \brief DIO 3 IRQ callback
 */
void SX1276OnDio3Irq( int chip );

/*!
 * \brief DIO 4 IRQ callback
 */
void SX1276OnDio4Irq( int chip );

/*!
 * \brief DIO 5 IRQ callback
 */
void SX1276OnDio5Irq( int chip );

/*!
 * \brief Tx & Rx timeout timer callback
 */
void SX1276OnTimeoutIrq( unsigned long chip );

/*
 * Private global constants
 */

/*!
 * Radio hardware registers initialization
 *
 * \remark RADIO_INIT_REGISTERS_VALUE is defined in sx1276-board.h file
 */
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] =
{
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Bandwidth
};

/*
 * Private global variables
 */

/*!
 * Radio callbacks variable
 */
static RadioEvents_t *RadioEvents;

/*!
 * Reception buffer
 */
static uint8_t TxBuffer[4][RX_BUFFER_SIZE];
static uint8_t RxBuffer[4][RX_BUFFER_SIZE];


/*
 * Public global variables
 */

/*!
 * Radio hardware and global parameters
 */
SX1276_t SX1276[4];

/*!
 * Hardware DIO IRQ callback initialization
 */
/*DioIrqHandler *DioIrq[] = { SX1276OnDio0Irq, SX1276OnDio1Irq,
                            SX1276OnDio2Irq, SX1276OnDio3Irq,
                            SX1276OnDio4Irq, NULL };
*/
/*!
 * Tx and Rx timers
 */
struct timer_list TxTimeoutTimer[4];
struct timer_list RxTimeoutTimer[4];
struct timer_list RxTimeoutSyncWord[4];
struct timeval oldtv;
extern struct workqueue_struct *RadioWorkQueue;

/*
 * Radio driver functions implementation
 */

void SX1276Init( int chip,RadioEvents_t *events )
{
    uint8_t i;
    uint8_t chipversion1;
    //printk("%s,%d\r\n",__func__,__LINE__);
    RadioEvents = events;

    // Initialize driver timeout timers

    // Initialize driver timeout timers
    init_timer(&TxTimeoutTimer[chip]);
    init_timer(&RxTimeoutTimer[chip]);
    init_timer(&RxTimeoutSyncWord[chip]);
    do_gettimeofday(&oldtv);
    TxTimeoutTimer[chip].function = SX1276OnTimeoutIrq;
    TxTimeoutTimer[chip].data = chip;
    RxTimeoutTimer[chip].function = SX1276OnTimeoutIrq;
    RxTimeoutTimer[chip].data = chip;
    RxTimeoutSyncWord[chip].function = SX1276OnTimeoutIrq;
    RxTimeoutSyncWord[chip].data = chip;
    SX1276IoInit(chip);
    SX1276Reset(chip );

    chipversion1 = spi_w8r8(SX1276[chip].Spi,0x42 & 0x7f);
    printk(KERN_INFO "sx1278_%d chipversion is 0x%02x\r\n",chip,chipversion1);

    RxChainCalibration(chip );

    SX1276SetOpMode(chip, RF_OPMODE_SLEEP );

    SX1276IoIrqInit(chip);

    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        SX1276SetModem(chip, RadioRegsInit[i].Modem );
        SX1276Write(chip, RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }

    SX1276SetModem(chip, MODEM_FSK );

    SX1276[chip].Settings.State = RF_IDLE;
}

RadioState_t SX1276GetStatus( int chip )
{
    return SX1276[chip].Settings.State;
}

void SX1276SetChannel( int chip,uint32_t freq )
{
    //printk("%s,chip = %d,freq = %d\r\n",__func__,chip,freq);
    //dump_stack();
#if 0
    SX1276[chip].Settings.Channel = freq;
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
#endif
    SX1276Write(chip, REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    SX1276Write(chip, REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    SX1276Write(chip, REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

bool SX1276IsChannelFree( int chip,RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    bool status = true;
    //int16_t rssi = 0;
    //uint32_t carrierSenseTime = 0;

    SX1276SetModem(chip, modem );

    SX1276SetChannel(chip, freq );

    SX1276SetOpMode(chip, RF_OPMODE_RECEIVER );

    udelay( 1000 );

    //carrierSenseTime = TimerGetCurrentTime( );

    // Perform carrier sense for maxCarrierSenseTime
    /*while( TimerGetElapsedTime( carrierSenseTime ) < maxCarrierSenseTime )
    {
        rssi = SX1276ReadRssi(chip, modem );

        if( rssi > rssiThresh )
        {
            status = false;
            break;
        }
    }*/
    SX1276SetSleep(chip );
    return status;
}

uint32_t SX1276Random( int chip )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation
     */
    // Set LoRa modem ON
    SX1276SetModem(chip, MODEM_LORA );

    // Disable LoRa modem interrupts
    SX1276Write(chip, REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    SX1276SetOpMode(chip, RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        udelay( 1000 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )SX1276Read(chip, REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    SX1276SetSleep(chip );

    return rnd;
}

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( int chip )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;
#if 1
    // Save context
    regPaConfigInitVal = SX1276Read(chip, REG_PACONFIG );
    initialFreq = ( ( ( uint32_t )SX1276Read(chip, REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )SX1276Read(chip, REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )SX1276Read(chip, REG_FRFLSB ) ) ) * FREQ_STEP;
#endif
    // Cut the PA just in case, RFO output, power = -1 dBm
    SX1276Write(chip, REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    SX1276Write(chip, REG_IMAGECAL, ( SX1276Read(chip, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1276Read(chip, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    SX1276SetChannel(chip, 868000000 );

    // Launch Rx chain calibration for HF band
    SX1276Write(chip, REG_IMAGECAL, ( SX1276Read(chip, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1276Read(chip, REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    SX1276Write(chip, REG_PACONFIG, regPaConfigInitVal );
    SX1276SetChannel(chip, initialFreq );
}
#if 0
/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
static uint8_t GetFskBandwidthRegValue( int chip,uint32_t bandwidth )
{
    uint8_t i;

    for( i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}
#endif
void SX1276SetRxConfig( int chip,RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    SX1276SetModem(chip, modem );
    /*printk("%s:\r\n\
            chip = %d,\r\n\
            modem = %d,\r\n\
            bandwidth = %d,\r\n\
            datarate = %d,\r\n\
            coderate = %d,\r\n\
            bandwidthAfc = %d,\r\n\
            preambleLen = %d,\r\n\
            symbTimeout = %d,\r\n\
            fixLen = %d,\r\n\
            payloadLen = %d,\r\n\
            crcOn = %d,\r\n\
            freqHopOn = %d,\r\n\
            hopPeriod = %d,\r\n\
            iqInverted = %d,\r\n\
            rxContinuous = %d\r\n",
            __func__,
            chip,
            modem,
            bandwidth,
            datarate,
            coderate,
            bandwidthAfc,
            preambleLen,
            symbTimeout,
            fixLen,
            payloadLen,
            crcOn,
            freqHopOn,
            hopPeriod,
            iqInverted,
            rxContinuous);*/
    switch( modem )
    {
    case MODEM_FSK:
        {
#if 0
            SX1276[chip].Settings.Fsk.Bandwidth = bandwidth;
            SX1276[chip].Settings.Fsk.Datarate = datarate;
            SX1276[chip].Settings.Fsk.BandwidthAfc = bandwidthAfc;
            SX1276[chip].Settings.Fsk.FixLen = fixLen;
            SX1276[chip].Settings.Fsk.PayloadLen = payloadLen;
            SX1276[chip].Settings.Fsk.CrcOn = crcOn;
            SX1276[chip].Settings.Fsk.IqInverted = iqInverted;
            SX1276[chip].Settings.Fsk.RxContinuous = rxContinuous;
            SX1276[chip].Settings.Fsk.PreambleLen = preambleLen;
            SX1276[chip].Settings.Fsk.RxSingleTimeout = ( uint32_t )( symbTimeout * ( ( 1.0 / ( double )datarate ) * 8.0 ) * 1000 );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            SX1276Write(chip, REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            SX1276Write(chip, REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            SX1276Write(chip, REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            SX1276Write(chip, REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            SX1276Write(chip, REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            SX1276Write(chip, REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                SX1276Write(chip, REG_PAYLOADLENGTH, payloadLen );
            }
            else
            {
                SX1276Write(chip, REG_PAYLOADLENGTH, 0xFF ); // Set payload length to the maximum
            }

            SX1276Write(chip, REG_PACKETCONFIG1,
                         ( SX1276Read(chip, REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            SX1276Write(chip, REG_PACKETCONFIG2, ( SX1276Read(chip, REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
#endif
        }
        break;
    case MODEM_LORA:
        {
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
            SX1276[chip].Settings.LoRa.Bandwidth = bandwidth;
            SX1276[chip].Settings.LoRa.Datarate = datarate;
            SX1276[chip].Settings.LoRa.Coderate = coderate;
            SX1276[chip].Settings.LoRa.PreambleLen = preambleLen;
            SX1276[chip].Settings.LoRa.FixLen = fixLen;
            SX1276[chip].Settings.LoRa.PayloadLen = payloadLen;
            SX1276[chip].Settings.LoRa.CrcOn = crcOn;
            SX1276[chip].Settings.LoRa.FreqHopOn = freqHopOn;
            SX1276[chip].Settings.LoRa.HopPeriod = hopPeriod;
            SX1276[chip].Settings.LoRa.IqInverted = iqInverted;
            SX1276[chip].Settings.LoRa.RxContinuous = rxContinuous;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }

            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276[chip].Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276[chip].Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            SX1276Write(chip, REG_LR_MODEMCONFIG1,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            SX1276Write(chip, REG_LR_MODEMCONFIG2,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) |
                           ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            SX1276Write(chip, REG_LR_MODEMCONFIG3,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1276[chip].Settings.LoRa.LowDatarateOptimize << 3 ) );

            SX1276Write(chip, REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );

            SX1276Write(chip, REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            SX1276Write(chip, REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                SX1276Write(chip, REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
            {
                SX1276Write(chip, REG_LR_PLLHOP, ( SX1276Read(chip, REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                SX1276Write(chip, REG_LR_HOPPERIOD, SX1276[chip].Settings.LoRa.HopPeriod );
            }

            if( ( bandwidth == 9 ) && ( SX1276[chip].Settings.Channel > RF_MID_BAND_THRESH ) )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1276Write(chip, REG_LR_TEST36, 0x02 );
                SX1276Write(chip, REG_LR_TEST3A, 0x64 );
            }
            else if( bandwidth == 9 )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1276Write(chip, REG_LR_TEST36, 0x02 );
                SX1276Write(chip, REG_LR_TEST3A, 0x7F );
            }
            else
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1276Write(chip, REG_LR_TEST36, 0x03 );
            }

            if( datarate == 6 )
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE,
                             ( SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                SX1276Write(chip, REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE,
                             ( SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                SX1276Write(chip, REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void SX1276SetTxConfig( int chip,RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    /*printk("%s:\r\n\
            chip = %d,\r\n\
            modem = %d,\r\n\
            power = %d,\r\n\
            fdev = %d,\r\n\
            bandwidth = %d,\r\n\
            datarate = %d,\r\n\
            coderate = %d,\r\n\
            preambleLen = %d,\r\n\
            fixLen = %d,\r\n\
            crcOn = %d,\r\n\
            freqHopOn = %d,\r\n\
            hopPeriod = %d,\r\n\
            iqInverted = %d,\r\n\
            timeout = %d\r\n",
            __func__,
            chip,
            modem,
            power,
            fdev,
            bandwidth,
            datarate,
            coderate,
            preambleLen,
            fixLen,
            crcOn,
            freqHopOn,
            hopPeriod,
            iqInverted,
            timeout);*/
    SX1276SetModem(chip, modem );

    SX1276SetRfTxPower(chip, power );

    switch( modem )
    {
    case MODEM_FSK:
        {
#if 0
            SX1276[chip].Settings.Fsk.Power = power;
            SX1276[chip].Settings.Fsk.Fdev = fdev;
            SX1276[chip].Settings.Fsk.Bandwidth = bandwidth;
            SX1276[chip].Settings.Fsk.Datarate = datarate;
            SX1276[chip].Settings.Fsk.PreambleLen = preambleLen;
            SX1276[chip].Settings.Fsk.FixLen = fixLen;
            SX1276[chip].Settings.Fsk.CrcOn = crcOn;
            SX1276[chip].Settings.Fsk.IqInverted = iqInverted;
            SX1276[chip].Settings.Fsk.TxTimeout = timeout;

            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );

            SX1276Write(chip, REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            SX1276Write(chip, REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            SX1276Write(chip, REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            SX1276Write(chip, REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            SX1276Write(chip, REG_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            SX1276Write(chip, REG_PREAMBLELSB, preambleLen & 0xFF );

            SX1276Write(chip, REG_PACKETCONFIG1,
                         ( SX1276Read(chip, REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            SX1276Write(chip, REG_PACKETCONFIG2, ( SX1276Read(chip, REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
#endif
        }
        break;
    case MODEM_LORA:
        {
            SX1276[chip].Settings.LoRa.Power = power;
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
            SX1276[chip].Settings.LoRa.Bandwidth = bandwidth;
            SX1276[chip].Settings.LoRa.Datarate = datarate;
            SX1276[chip].Settings.LoRa.Coderate = coderate;
            SX1276[chip].Settings.LoRa.PreambleLen = preambleLen;
            SX1276[chip].Settings.LoRa.FixLen = fixLen;
            SX1276[chip].Settings.LoRa.FreqHopOn = freqHopOn;
            SX1276[chip].Settings.LoRa.HopPeriod = hopPeriod;
            SX1276[chip].Settings.LoRa.CrcOn = crcOn;
            SX1276[chip].Settings.LoRa.IqInverted = iqInverted;
            SX1276[chip].Settings.LoRa.TxTimeout = timeout;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276[chip].Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276[chip].Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
            {
                SX1276Write(chip, REG_LR_PLLHOP, ( SX1276Read(chip, REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                SX1276Write(chip, REG_LR_HOPPERIOD, SX1276[chip].Settings.LoRa.HopPeriod );
            }

            SX1276Write(chip, REG_LR_MODEMCONFIG1,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            SX1276Write(chip, REG_LR_MODEMCONFIG2,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) );

            SX1276Write(chip, REG_LR_MODEMCONFIG3,
                         ( SX1276Read(chip, REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1276[chip].Settings.LoRa.LowDatarateOptimize << 3 ) );

            SX1276Write(chip, REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            SX1276Write(chip, REG_LR_PREAMBLELSB, preambleLen & 0xFF );

            if( datarate == 6 )
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE,
                             ( SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                SX1276Write(chip, REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE,
                             ( SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                SX1276Write(chip, REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

uint32_t SX1276GetTimeOnAir( int chip,RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;
#if 0
    switch( modem )
    {
    case MODEM_FSK:
        {
            airTime = round( ( 8 * ( SX1276[chip].Settings.Fsk.PreambleLen +
                                     ( ( SX1276Read(chip, REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( SX1276[chip].Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( SX1276Read(chip, REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( SX1276[chip].Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                                     SX1276[chip].Settings.Fsk.Datarate ) * 1000 );
        }
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( SX1276[chip].Settings.LoRa.Bandwidth )
            {
            //case 0: // 7.8 kHz
            //    bw = 7800;
            //    break;
            //case 1: // 10.4 kHz
            //    bw = 10400;
            //    break;
            //case 2: // 15.6 kHz
            //    bw = 15600;
            //    break;
            //case 3: // 20.8 kHz
            //    bw = 20800;
            //    break;
            //case 4: // 31.2 kHz
            //    bw = 31200;
            //    break;
            //case 5: // 41.4 kHz
            //    bw = 41400;
            //    break;
            //case 6: // 62.5 kHz
            //    bw = 62500;
            //    break;
            case 7: // 125 kHz
                bw = 125000;
                break;
            case 8: // 250 kHz
                bw = 250000;
                break;
            case 9: // 500 kHz
                bw = 500000;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << SX1276[chip].Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( SX1276[chip].Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * SX1276[chip].Settings.LoRa.Datarate +
                                 28 + 16 * SX1276[chip].Settings.LoRa.CrcOn -
                                 ( SX1276[chip].Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * ( SX1276[chip].Settings.LoRa.Datarate -
                                 ( ( SX1276[chip].Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                                 ( SX1276[chip].Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return ms secs
            airTime = floor( tOnAir * 1000 + 0.999 );
        }
        break;
    }
#endif
    return airTime;
}

void SX1276Send( int chip,uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        {
            SX1276[chip].Settings.FskPacketHandler.NbBytes = 0;
            SX1276[chip].Settings.FskPacketHandler.Size = size;

            if( SX1276[chip].Settings.Fsk.FixLen == false )
            {
                SX1276WriteFifo(chip, ( uint8_t* )&size, 1 );
            }
            else
            {
                SX1276Write(chip, REG_PAYLOADLENGTH, size );
            }

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                SX1276[chip].Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                memcpy1( TxBuffer[chip], buffer, size );
                SX1276[chip].Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            SX1276WriteFifo(chip, buffer, SX1276[chip].Settings.FskPacketHandler.ChunkSize );
            SX1276[chip].Settings.FskPacketHandler.NbBytes += SX1276[chip].Settings.FskPacketHandler.ChunkSize;
            txTimeout = SX1276[chip].Settings.Fsk.TxTimeout;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276[chip].Settings.LoRa.IqInverted == true )
            {
                SX1276Write(chip, REG_LR_INVERTIQ, ( ( SX1276Read(chip, REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                SX1276Write(chip, REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                SX1276Write(chip, REG_LR_INVERTIQ, ( ( SX1276Read(chip, REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                SX1276Write(chip, REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            SX1276[chip].Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            SX1276Write(chip, REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            SX1276Write(chip, REG_LR_FIFOTXBASEADDR, 0 );
            SX1276Write(chip, REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( SX1276Read(chip, REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                SX1276SetStby( chip);
                udelay( 1000 );
            }
            // Write payload buffer
            SX1276WriteFifo(chip, buffer, size );
            txTimeout = SX1276[chip].Settings.LoRa.TxTimeout;
        }
        break;
    }

    SX1276SetTx( chip,txTimeout );
}

void SX1276SetSleep( int chip )
{
    del_timer( &RxTimeoutTimer[chip] );
    del_timer( &TxTimeoutTimer[chip] );

    SX1276SetOpMode(chip, RF_OPMODE_SLEEP );
    SX1276[chip].Settings.State = RF_IDLE;
}

void SX1276SetStby( int chip )
{
    del_timer( &RxTimeoutTimer[chip] );
    del_timer( &TxTimeoutTimer[chip] );

    SX1276SetOpMode(chip, RF_OPMODE_STANDBY );
    SX1276[chip].Settings.State = RF_IDLE;
}

void SX1276SetRx( int chip,uint32_t timeout )
{
    bool rxContinuous = false;
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        {
            rxContinuous = SX1276[chip].Settings.Fsk.RxContinuous;

            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO1_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );

            SX1276Write(chip, REG_DIOMAPPING2, ( SX1276Read(chip, REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) |
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );

            SX1276[chip].Settings.FskPacketHandler.FifoThresh = SX1276Read(chip, REG_FIFOTHRESH ) & 0x3F;

            SX1276Write(chip, REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT );

            SX1276[chip].Settings.FskPacketHandler.PreambleDetected = false;
            SX1276[chip].Settings.FskPacketHandler.SyncWordDetected = false;
            SX1276[chip].Settings.FskPacketHandler.NbBytes = 0;
            SX1276[chip].Settings.FskPacketHandler.Size = 0;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276[chip].Settings.LoRa.IqInverted == true )
            {
                SX1276Write(chip, REG_LR_INVERTIQ, ( ( SX1276Read(chip, REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                SX1276Write(chip, REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                SX1276Write(chip, REG_LR_INVERTIQ, ( ( SX1276Read(chip, REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                SX1276Write(chip, REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( SX1276[chip].Settings.LoRa.Bandwidth < 9 )
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE, SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) & 0x7F );
                SX1276Write(chip, REG_LR_TEST30, 0x00 );
                switch( SX1276[chip].Settings.LoRa.Bandwidth )
                {
                case 0: // 7.8 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x48 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 7810 );
                    break;
                case 1: // 10.4 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x44 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 10420 );
                    break;
                case 2: // 15.6 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x44 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 15620 );
                    break;
                case 3: // 20.8 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x44 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 20830 );
                    break;
                case 4: // 31.2 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x44 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 31250 );
                    break;
                case 5: // 41.4 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x44 );
                    SX1276SetChannel(chip,SX1276[chip].Settings.Channel + 41670 );
                    break;
                case 6: // 62.5 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x40 );
                    break;
                case 7: // 125 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x40 );
                    break;
                case 8: // 250 kHz
                    SX1276Write(chip, REG_LR_TEST2F, 0x40 );
                    break;
                }
            }
            else
            {
                SX1276Write(chip, REG_LR_DETECTOPTIMIZE, SX1276Read(chip, REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }

            rxContinuous = SX1276[chip].Settings.LoRa.RxContinuous;

            if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
            {
                SX1276Write(chip, REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                SX1276Write(chip, REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            SX1276Write(chip, REG_LR_FIFORXBASEADDR, 0 );
            SX1276Write(chip, REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    memset( RxBuffer[chip], 0, ( size_t )RX_BUFFER_SIZE );

    SX1276[chip].Settings.State = RF_RX_RUNNING;
    if( timeout != 0 )
    {
        RxTimeoutTimer[chip].expires = jiffies + timeout;
        add_timer( &RxTimeoutTimer[chip] );
    }

    if( SX1276[chip].Settings.Modem == MODEM_FSK )
    {
        SX1276SetOpMode(chip, RF_OPMODE_RECEIVER );

        if( rxContinuous == false )
        {
            RxTimeoutSyncWord[chip].expires = jiffies + SX1276[chip].Settings.Fsk.RxSingleTimeout;
            add_timer( &RxTimeoutSyncWord[chip] );
        }
    }
    else
    {
        if( rxContinuous == true )
        {
            SX1276SetOpMode(chip, RFLR_OPMODE_RECEIVER );
        }
        else
        {
            SX1276SetOpMode(chip, RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void SX1276SetTx( int chip,uint32_t timeout )
{
    TxTimeoutTimer[chip].expires = jiffies + timeout;
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO1_01 );

            SX1276Write(chip, REG_DIOMAPPING2, ( SX1276Read(chip, REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            SX1276[chip].Settings.FskPacketHandler.FifoThresh = SX1276Read(chip, REG_FIFOTHRESH ) & 0x3F;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
            {
                SX1276Write(chip, REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone, DIO2=FhssChangeChannel
                SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                SX1276Write(chip, REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }

    SX1276[chip].Settings.State = RF_TX_RUNNING;
    add_timer( &TxTimeoutTimer[chip] );
    SX1276SetOpMode(chip, RF_OPMODE_TRANSMITTER );
}

void SX1276StartCad( int chip )
{
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        {

        }
        break;
    case MODEM_LORA:
        {
            SX1276Write(chip, REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED
                                        );

            // DIO3=CADDone
            SX1276Write(chip, REG_DIOMAPPING1, ( SX1276Read(chip, REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO3_MASK ) | RFLR_DIOMAPPING1_DIO3_00 );

            SX1276[chip].Settings.State = RF_CAD;
            SX1276SetOpMode(chip, RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

void SX1276SetTxContinuousWave( int chip,uint32_t freq, int8_t power, uint16_t time )
{
    uint32_t timeout = ( uint32_t )( time * 1000 );

    SX1276SetChannel(chip, freq );

    SX1276SetTxConfig(chip, MODEM_FSK, power, 0, 0, 4800, 0, 5, false, false, 0, 0, 0, timeout );

    SX1276Write(chip, REG_PACKETCONFIG2, ( SX1276Read(chip, REG_PACKETCONFIG2 ) & RF_PACKETCONFIG2_DATAMODE_MASK ) );
    // Disable radio interrupts
    SX1276Write(chip, REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_11 | RF_DIOMAPPING1_DIO1_11 );
    SX1276Write(chip, REG_DIOMAPPING2, RF_DIOMAPPING2_DIO4_10 | RF_DIOMAPPING2_DIO5_10 );

    TxTimeoutTimer[chip].expires = jiffies + timeout;
    SX1276[chip].Settings.State = RF_TX_RUNNING;
    add_timer( &TxTimeoutTimer[chip] );
    SX1276SetOpMode(chip, RF_OPMODE_TRANSMITTER );
}

int16_t SX1276ReadRssi( int chip,RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( SX1276Read(chip, REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( SX1276[chip].Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + SX1276Read(chip, REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + SX1276Read(chip, REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}

void SX1276Reset( int chip )
{
    switch(chip)
    {
        case 0:
        gpio_set_value(SX1278_1_RST_PIN,0);
        //gpio_set_value(SX1278_2_RST_PIN,0);
        udelay(1000);
        gpio_set_value(SX1278_1_RST_PIN,1);
        //gpio_set_value(SX1278_2_RST_PIN,1);
        udelay(6000);
        break;
        case 1:
        gpio_set_value(SX1278_2_RST_PIN,0);
        udelay(1000);
        gpio_set_value(SX1278_2_RST_PIN,1);
        udelay(6000);
        break;
        case 2:
        gpio_set_value(SX1278_3_RST_PIN,0);
        udelay(1000);
        gpio_set_value(SX1278_3_RST_PIN,1);
        udelay(6000);
        break;
        case 3:
        gpio_set_value(SX1278_4_RST_PIN,0);
        udelay(1000);
        gpio_set_value(SX1278_4_RST_PIN,1);
        udelay(6000);
        break;
        default:
        break;
    }
}

void SX1276SetOpMode( int chip,uint8_t opMode )
{
    if( opMode == RF_OPMODE_SLEEP )
    {
        SX1276SetAntSwLowPower(chip, true );
    }
    else
    {
        SX1276SetAntSwLowPower(chip, false );
        SX1276SetAntSw(chip, opMode );
    }
    SX1276Write(chip, REG_OPMODE, ( SX1276Read(chip, REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
}

void SX1276SetModem( int chip,RadioModems_t modem )
{
    if( ( SX1276Read(chip, REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 )
    {
        SX1276[chip].Settings.Modem = MODEM_LORA;
    }
    else
    {
        SX1276[chip].Settings.Modem = MODEM_FSK;
    }

    if( SX1276[chip].Settings.Modem == modem )
    {
        return;
    }

    SX1276[chip].Settings.Modem = modem;
    switch( SX1276[chip].Settings.Modem )
    {
    default:
    case MODEM_FSK:
        SX1276SetSleep(chip );
        SX1276Write(chip, REG_OPMODE, ( SX1276Read(chip, REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

        SX1276Write(chip, REG_DIOMAPPING1, 0x00 );
        SX1276Write(chip, REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
        break;
    case MODEM_LORA:
        SX1276SetSleep(chip );
        SX1276Write(chip, REG_OPMODE, ( SX1276Read(chip, REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        SX1276Write(chip, REG_DIOMAPPING1, 0x00 );
        SX1276Write(chip, REG_DIOMAPPING2, 0x00 );
        break;
    }
}

void SX1276Write( int chip,uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer(chip, addr, &data, 1 );
}

uint8_t SX1276Read( int chip,uint8_t addr )
{
    uint8_t data;
    SX1276ReadBuffer(chip, addr, &data, 1 );
    return data;
}

void SX1276WriteBuffer( int chip,uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t *data = kmalloc(size + 1,GFP_KERNEL);
    data[0] = addr | 0x80;
    memcpy(data + 1,buffer,size);
    //spin_lock_bh(&spi_lock);
    spi_write(SX1276[chip].Spi,data,size + 1);
    //spin_unlock_bh(&spi_lock);
    kfree(data);
}

void SX1276ReadBuffer( int chip,uint8_t addr, uint8_t *buffer, uint8_t size )
{
    addr = addr & 0x7F;
    //spin_lock_bh(&spi_lock);
    spi_write_then_read(SX1276[chip].Spi,&addr,1,buffer,size);
    //spin_unlock_bh(&spi_lock);
}

void SX1276WriteFifo( int chip,uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer(chip, 0, buffer, size );
}

void SX1276ReadFifo( int chip,uint8_t *buffer, uint8_t size )
{
    SX1276ReadBuffer(chip, 0, buffer, size );
}

void SX1276SetMaxPayloadLength( int chip,RadioModems_t modem, uint8_t max )
{
    SX1276SetModem(chip, modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( SX1276[chip].Settings.Fsk.FixLen == false )
        {
            SX1276Write(chip, REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        SX1276Write(chip, REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}

void SX1276SetPublicNetwork( int chip,bool enable )
{
    printk("%s,%d\r\n",__func__,enable);
    SX1276SetModem(chip, MODEM_LORA );
    SX1276[chip].Settings.LoRa.PublicNetwork = enable;
    if( enable == true )
    {
        // Change LoRa modem SyncWord
        SX1276Write(chip, REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        SX1276Write(chip, REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}

void SX1276OnTimeoutIrqWorkQueue(struct work_struct *p_work)
{
    uint8_t i = 0;
    unsigned long chip;
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
    chip = pstMyWork->param;
    printk("%s, %d\r\n",__func__,(int)chip);
    mutex_lock(&RadioChipMutex[chip]);
    switch( SX1276[chip].Settings.State )
    {
    case RF_RX_RUNNING:
        if( SX1276[chip].Settings.Modem == MODEM_FSK )
        {
            SX1276[chip].Settings.FskPacketHandler.PreambleDetected = false;
            SX1276[chip].Settings.FskPacketHandler.SyncWordDetected = false;
            SX1276[chip].Settings.FskPacketHandler.NbBytes = 0;
            SX1276[chip].Settings.FskPacketHandler.Size = 0;

            // Clear Irqs
            SX1276Write(chip, REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
                                        RF_IRQFLAGS1_PREAMBLEDETECT |
                                        RF_IRQFLAGS1_SYNCADDRESSMATCH );
            SX1276Write(chip, REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

            if( SX1276[chip].Settings.Fsk.RxContinuous == true )
            {
                // Continuous mode restart Rx chain
                SX1276Write(chip, REG_RXCONFIG, SX1276Read(chip, REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                add_timer( &RxTimeoutSyncWord[chip] );
            }
            else
            {
                SX1276[chip].Settings.State = RF_IDLE;
                del_timer( &RxTimeoutSyncWord[chip] );
            }
        }
        if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
        {
            RadioEvents->RxTimeout(chip );
        }
        break;
    case RF_TX_RUNNING:
        // Tx timeout shouldn't happen.
        // But it has been observed that when it happens it is a result of a corrupted SPI transfer
        // it depends on the platform design.
        //
        // The workaround is to put the radio in a known state. Thus, we re-initialize it.

        // BEGIN WORKAROUND
        del_timer(&TxTimeoutTimer[chip]);
        // Reset the radio
        SX1276Reset(chip );

        // Calibrate Rx chain
        RxChainCalibration(chip );

        // Initialize radio default values
        SX1276SetOpMode(chip, RF_OPMODE_SLEEP );

        for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
        {
            SX1276SetModem(chip, RadioRegsInit[i].Modem );
            SX1276Write(chip, RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
        }
        SX1276SetModem(chip, MODEM_FSK );

        // Restore previous network type setting.
        SX1276SetPublicNetwork( chip,SX1276[chip].Settings.LoRa.PublicNetwork );
        // END WORKAROUND

        SX1276[chip].Settings.State = RF_IDLE;
        if( ( RadioEvents != NULL ) && ( RadioEvents->TxTimeout != NULL ) )
        {
            RadioEvents->TxTimeout(chip );
        }
        break;
    default:
        break;
    }
    mutex_unlock(&RadioChipMutex[chip]);
}

static st_MyWork stMyWorkChip1,stMyWorkChip2,stMyWorkChip3,stMyWorkChip4;
//DECLARE_TASKLET(sx1276_1OnTimeoutIrq,SX1276OnTimeoutIrqTasklet,0);
//DECLARE_TASKLET(sx1276_2OnTimeoutIrq,SX1276OnTimeoutIrqTasklet,1);
//DECLARE_TASKLET(sx1276_3OnTimeoutIrq,SX1276OnTimeoutIrqTasklet,2);
//DECLARE_TASKLET(sx1276_4OnTimeoutIrq,SX1276OnTimeoutIrqTasklet,3);

void SX1276OnTimeoutIrq( unsigned long chip )
{
    switch(chip)
    {
        case 0:
            stMyWorkChip1.param = chip;
            INIT_WORK(&(stMyWorkChip1.save), SX1276OnTimeoutIrqWorkQueue);
            queue_work(RadioWorkQueue, &(stMyWorkChip1.save));
            //tasklet_schedule(&sx1276_1OnTimeoutIrq);
            break;
        case 1:
            stMyWorkChip2.param = chip;
            INIT_WORK(&(stMyWorkChip2.save), SX1276OnTimeoutIrqWorkQueue);
            queue_work(RadioWorkQueue, &(stMyWorkChip2.save));
            //tasklet_schedule(&sx1276_2OnTimeoutIrq);
            break;
        case 2:
            stMyWorkChip3.param = chip;
            INIT_WORK(&(stMyWorkChip3.save), SX1276OnTimeoutIrqWorkQueue);
            queue_work(RadioWorkQueue, &(stMyWorkChip3.save));
            //tasklet_schedule(&sx1276_3OnTimeoutIrq);
            break;
        case 3:
            stMyWorkChip4.param = chip;
            INIT_WORK(&(stMyWorkChip4.save), SX1276OnTimeoutIrqWorkQueue);
            queue_work(RadioWorkQueue, &(stMyWorkChip4.save));
            //tasklet_schedule(&sx1276_4OnTimeoutIrq);
            break;
        default:
            break;
    }
    
    
}

void SX1276OnDio0Irq(struct work_struct *p_work)
{
    unsigned long chip;
    volatile uint8_t irqFlags = 0;
    int16_t rssi;
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
    chip = pstMyWork->param;
    mutex_lock(&RadioChipMutex[chip]);
    //printk("%s,%d\r\n",__func__,chip);
    switch( SX1276[chip].Settings.State )
    {
        case RF_RX_RUNNING:
            //del_timer( &RxTimeoutTimer[chip] );
            // RxDone interrupt
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_FSK:
                if( SX1276[chip].Settings.Fsk.CrcOn == true )
                {
                    irqFlags = SX1276Read(chip, REG_IRQFLAGS2 );
                    if( ( irqFlags & RF_IRQFLAGS2_CRCOK ) != RF_IRQFLAGS2_CRCOK )
                    {
                        // Clear Irqs
                        SX1276Write(chip, REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
                                                    RF_IRQFLAGS1_PREAMBLEDETECT |
                                                    RF_IRQFLAGS1_SYNCADDRESSMATCH );
                        SX1276Write(chip, REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

                        del_timer( &RxTimeoutTimer[chip] );

                        if( SX1276[chip].Settings.Fsk.RxContinuous == false )
                        {
                            del_timer( &RxTimeoutSyncWord[chip] );
                            SX1276[chip].Settings.State = RF_IDLE;
                        }
                        else
                        {
                            // Continuous mode restart Rx chain
                            SX1276Write(chip, REG_RXCONFIG, SX1276Read(chip, REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                            add_timer( &RxTimeoutSyncWord[chip] );
                        }

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError(chip );
                        }
                        SX1276[chip].Settings.FskPacketHandler.PreambleDetected = false;
                        SX1276[chip].Settings.FskPacketHandler.SyncWordDetected = false;
                        SX1276[chip].Settings.FskPacketHandler.NbBytes = 0;
                        SX1276[chip].Settings.FskPacketHandler.Size = 0;
                        break;
                    }
                }

                // Read received packet size
                if( ( SX1276[chip].Settings.FskPacketHandler.Size == 0 ) && ( SX1276[chip].Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( SX1276[chip].Settings.Fsk.FixLen == false )
                    {
                        SX1276ReadFifo(chip, ( uint8_t* )&SX1276[chip].Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        SX1276[chip].Settings.FskPacketHandler.Size = SX1276Read(chip, REG_PAYLOADLENGTH );
                    }
                    SX1276ReadFifo(chip, RxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes, SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += ( SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                }
                else
                {
                    SX1276ReadFifo(chip, RxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes, SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += ( SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                }

                del_timer( &RxTimeoutTimer[chip] );

                if( SX1276[chip].Settings.Fsk.RxContinuous == false )
                {
                    SX1276[chip].Settings.State = RF_IDLE;
                    del_timer( &RxTimeoutSyncWord[chip] );
                }
                else
                {
                    // Continuous mode restart Rx chain
                    SX1276Write(chip, REG_RXCONFIG, SX1276Read(chip, REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                    add_timer( &RxTimeoutSyncWord[chip] );
                }

                if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                {
                    RadioEvents->RxDone(chip, RxBuffer[chip], SX1276[chip].Settings.FskPacketHandler.Size, SX1276[chip].Settings.FskPacketHandler.RssiValue, 0 );
                }
                SX1276[chip].Settings.FskPacketHandler.PreambleDetected = false;
                SX1276[chip].Settings.FskPacketHandler.SyncWordDetected = false;
                SX1276[chip].Settings.FskPacketHandler.NbBytes = 0;
                SX1276[chip].Settings.FskPacketHandler.Size = 0;
                break;
            case MODEM_LORA:
                {
                    int8_t snr = 0;

                    // Clear Irq
                    SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );

                    irqFlags = SX1276Read(chip, REG_LR_IRQFLAGS );
                    if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
                    {
                        // Clear Irq
                        SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );

                        if( SX1276[chip].Settings.LoRa.RxContinuous == false )
                        {
                            SX1276[chip].Settings.State = RF_IDLE;
                        }
                        del_timer( &RxTimeoutTimer[chip] );

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError(chip );
                        }
                        break;
                    }

                    SX1276[chip].Settings.LoRaPacketHandler.SnrValue = SX1276Read(chip, REG_LR_PKTSNRVALUE );
                    if( SX1276[chip].Settings.LoRaPacketHandler.SnrValue & 0x80 ) // The SNR sign bit is 1
                    {
                        // Invert and divide by 4
                        snr = ( ( ~SX1276[chip].Settings.LoRaPacketHandler.SnrValue + 1 ) & 0xFF ) >> 2;
                        snr = -snr;
                    }
                    else
                    {
                        // Divide by 4
                        snr = ( SX1276[chip].Settings.LoRaPacketHandler.SnrValue & 0xFF ) >> 2;
                    }

                    rssi = SX1276Read(chip, REG_LR_PKTRSSIVALUE );
                    if( snr < 0 )
                    {
                        if( SX1276[chip].Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            SX1276[chip].Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                        else
                        {
                            SX1276[chip].Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                    }
                    else
                    {
                        if( SX1276[chip].Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            SX1276[chip].Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 );
                        }
                        else
                        {
                            SX1276[chip].Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 );
                        }
                    }

                    SX1276[chip].Settings.LoRaPacketHandler.Size = SX1276Read(chip, REG_LR_RXNBBYTES );
                    SX1276Write(chip, REG_LR_FIFOADDRPTR, SX1276Read(chip, REG_LR_FIFORXCURRENTADDR ) );
                    SX1276ReadFifo(chip, RxBuffer[chip], SX1276[chip].Settings.LoRaPacketHandler.Size );

                    if( SX1276[chip].Settings.LoRa.RxContinuous == false )
                    {
                        SX1276[chip].Settings.State = RF_IDLE;
                    }
                    del_timer( &RxTimeoutTimer[chip] );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                    {
                        RadioEvents->RxDone(chip, RxBuffer[chip], SX1276[chip].Settings.LoRaPacketHandler.Size, SX1276[chip].Settings.LoRaPacketHandler.RssiValue, SX1276[chip].Settings.LoRaPacketHandler.SnrValue );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            del_timer( &TxTimeoutTimer[chip] );
            // TxDone interrupt
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_LORA:
                // Clear Irq
                SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
                // Intentional fall through
            case MODEM_FSK:
            default:
                SX1276[chip].Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->TxDone != NULL ) )
                {
                    RadioEvents->TxDone(chip );
                }
                break;
            }
            break;
        default:
            break;
    }
    mutex_unlock(&RadioChipMutex[chip]);
}

void SX1276OnDio1Irq(struct work_struct *p_work)
{
    unsigned long chip;
    pst_MyWork pstMyWork = container_of(p_work, st_MyWork, save);
    chip = pstMyWork->param;
    //printk("%s,%d\r\n",__func__,chip);
    mutex_lock(&RadioChipMutex[chip]);
    switch( SX1276[chip].Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if( ( SX1276[chip].Settings.FskPacketHandler.Size == 0 ) && ( SX1276[chip].Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( SX1276[chip].Settings.Fsk.FixLen == false )
                    {
                        SX1276ReadFifo(chip, ( uint8_t* )&SX1276[chip].Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        SX1276[chip].Settings.FskPacketHandler.Size = SX1276Read(chip, REG_PAYLOADLENGTH );
                    }
                }

                if( ( SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes ) > SX1276[chip].Settings.FskPacketHandler.FifoThresh )
                {
                    SX1276ReadFifo(chip, ( RxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes ), SX1276[chip].Settings.FskPacketHandler.FifoThresh );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += SX1276[chip].Settings.FskPacketHandler.FifoThresh;
                }
                else
                {
                    SX1276ReadFifo(chip, ( RxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes ), SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += ( SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                }
                break;
            case MODEM_LORA:
                // Sync time out
                del_timer( &RxTimeoutTimer[chip] );
                // Clear Irq
                SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT );

                SX1276[chip].Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
                {
                    RadioEvents->RxTimeout(chip );
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_FSK:
                // FifoEmpty interrupt
                if( ( SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes ) > SX1276[chip].Settings.FskPacketHandler.ChunkSize )
                {
                    SX1276WriteFifo(chip, ( TxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes ), SX1276[chip].Settings.FskPacketHandler.ChunkSize );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += SX1276[chip].Settings.FskPacketHandler.ChunkSize;
                }
                else
                {
                    // Write the last chunk of data
                    SX1276WriteFifo(chip, TxBuffer[chip] + SX1276[chip].Settings.FskPacketHandler.NbBytes, SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes );
                    SX1276[chip].Settings.FskPacketHandler.NbBytes += SX1276[chip].Settings.FskPacketHandler.Size - SX1276[chip].Settings.FskPacketHandler.NbBytes;
                }
                break;
            case MODEM_LORA:
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
    mutex_unlock(&RadioChipMutex[chip]);
}

void SX1276OnDio2Irq( int chip )
{
    //printk("%s,%d\r\n",__func__,chip);
    switch( SX1276[chip].Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_FSK:
                // Checks if DIO4 is connected. If it is not PreambleDetected is set to true.
                //if( SX1276.DIO4.port == NULL )
                {
                    SX1276[chip].Settings.FskPacketHandler.PreambleDetected = true;
                }

                if( ( SX1276[chip].Settings.FskPacketHandler.PreambleDetected == true ) && ( SX1276[chip].Settings.FskPacketHandler.SyncWordDetected == false ) )
                {
                    del_timer( &RxTimeoutSyncWord[chip] );

                    SX1276[chip].Settings.FskPacketHandler.SyncWordDetected = true;

                    SX1276[chip].Settings.FskPacketHandler.RssiValue = -( SX1276Read(chip, REG_RSSIVALUE ) >> 1 );
#if 0
                    SX1276[chip].Settings.FskPacketHandler.AfcValue = ( int32_t )( double )( ( ( uint16_t )SX1276Read(chip, REG_AFCMSB ) << 8 ) |
                                                                           ( uint16_t )SX1276Read(chip, REG_AFCLSB ) ) *
                                                                           ( double )FREQ_STEP;
#endif
                    SX1276[chip].Settings.FskPacketHandler.RxGain = ( SX1276Read(chip, REG_LNA ) >> 5 ) & 0x07;
                }
                break;
            case MODEM_LORA:
                if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel(chip, ( SX1276Read(chip, REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1276[chip].Settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if( SX1276[chip].Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel(chip, ( SX1276Read(chip, REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

void SX1276OnDio3Irq( int chip )
{
    //printk("%s,%d\r\n",__func__,chip);
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        if( ( SX1276Read(chip, REG_LR_IRQFLAGS ) & RFLR_IRQFLAGS_CADDETECTED ) == RFLR_IRQFLAGS_CADDETECTED )
        {
            // Clear Irq
            SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone(chip, true );
            }
        }
        else
        {
            // Clear Irq
            SX1276Write(chip, REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone(chip, false );
            }
        }
        break;
    default:
        break;
    }
}

void SX1276OnDio4Irq( int chip )
{
    //printk("%s,%d\r\n",__func__,chip);
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        {
            if( SX1276[chip].Settings.FskPacketHandler.PreambleDetected == false )
            {
                SX1276[chip].Settings.FskPacketHandler.PreambleDetected = true;
            }
        }
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void SX1276OnDio5Irq( int chip )
{
    switch( SX1276[chip].Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}


