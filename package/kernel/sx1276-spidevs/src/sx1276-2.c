/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic sx1276_2 driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Wael Guibene
*/
#include <linux/string.h>
#include "radio.h"
#include "sx1276-2.h"
#include "sx1276-2-board.h"
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include "pinmap.h"
#include <linux/spinlock.h>

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
static void RxChainCalibration( void );

/*!
 * \brief Resets the sx1276_2
 */
void sx1276_2Reset( void );

/*!
 * \brief Sets the sx1276_2 in transmission mode for the given time
 * \param [IN] timeout Transmission timeout [us] [0: continuous, others timeout]
 */
void sx1276_2SetTx( uint32_t timeout );

/*!
 * \brief Writes the buffer contents to the sx1276_2 FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void sx1276_2WriteFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads the contents of the sx1276_2 FIFO
 *
 * \param [OUT] buffer Buffer where to copy the FIFO read data.
 * \param [IN] size Number of bytes to be read from the FIFO
 */
void sx1276_2ReadFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the sx1276_2 operating mode
 *
 * \param [IN] opMode New operating mode
 */
void sx1276_2SetOpMode( uint8_t opMode );

/*
 * sx1276_2 DIO IRQ callback functions prototype
 */

/*!
 * \brief DIO 0 IRQ callback
 */
void sx1276_2OnDio0Irq( unsigned long );

/*!
 * \brief DIO 1 IRQ callback
 */
void sx1276_2OnDio1Irq( unsigned long );

/*!
 * \brief DIO 2 IRQ callback
 */
void sx1276_2OnDio2Irq( unsigned long );

/*!
 * \brief DIO 3 IRQ callback
 */
void sx1276_2OnDio3Irq( unsigned long );

/*!
 * \brief DIO 4 IRQ callback
 */
void sx1276_2OnDio4Irq( unsigned long );

/*!
 * \brief DIO 5 IRQ callback
 */
void sx1276_2OnDio5Irq( unsigned long );

/*!
 * \brief Tx & Rx timeout timer callback
 */
void sx1276_2OnTimeoutIrq( unsigned long arg );

/*
 * Private global constants
 */

/*!
 * Radio hardware registers initialization
 * 
 * \remark RADIO_INIT_REGISTERS_VALUE is defined in sx1276_2-board.h file
 */
static const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

/*!
 * Precomputed FSK bandwidth registers values
 */
static const FskBandwidth_t FskBandwidths[] =
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
    { 300000, 0x00 }, // Invalid Badwidth
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
static uint8_t RxBuffer[RX_BUFFER_SIZE];

/*
 * Public global variables
 */

/*!
 * Radio hardware and global parameters
 */
sx1276_2_t sx1276_2;

/*!
 * Hardware DIO IRQ callback initialization
 */
/*static DioIrqHandler *DioIrq[] = { sx1276_2OnDio0Irq, sx1276_2OnDio1Irq,
                            sx1276_2OnDio2Irq, sx1276_2OnDio3Irq,
                            sx1276_2OnDio4Irq, NULL };
*/
/*!
 * Tx and Rx timers
 */
static struct timer_list TxTimeoutTimer;
static struct timer_list RxTimeoutTimer;
static struct timer_list RxTimeoutSyncWord;
static struct timeval oldtv;

extern spinlock_t spi_lock;
/*
 * Radio driver functions implementation
 */

void sx1276_2Init( RadioEvents_t *events )
{
    uint8_t i;
    uint8_t chipversion2;
    RadioEvents = events;

    // Initialize driver timeout timers
    init_timer(&TxTimeoutTimer);
    init_timer(&RxTimeoutTimer);
    init_timer(&RxTimeoutSyncWord);
    do_gettimeofday(&oldtv);
    TxTimeoutTimer.function = sx1276_2OnTimeoutIrq;
    RxTimeoutTimer.function = sx1276_2OnTimeoutIrq;
    RxTimeoutSyncWord.function = sx1276_2OnTimeoutIrq;
    //TxTimeoutTimer.expires = jiffies + HZ / 10;        //定时时间
    //add_timer(&TxTimeoutTimer);        //注册定时器
    //TxTimeoutTimer.expires = jiffies + HZ / 10;        //定时时间
    //add_timer(&RxTimeoutTimer);        //注册定时器
    //TxTimeoutTimer.expires = jiffies + HZ / 10;        //定时时间
    //add_timer(&RxTimeoutSyncWord);        //注册定时器
    sx1276_2IoInit();
    sx1276_2Reset( );
    //spin_lock_bh(&spi_lock);
    chipversion2 = spi_w8r8(sx1276_2.Spi,0x42 & 0x7f);
    //spin_unlock_bh(&spi_lock);
    printk(KERN_INFO "sx1278_2 chipversion is 0x%02x\r\n",chipversion2);
    RxChainCalibration( );
    
    sx1276_2SetOpMode( RF_OPMODE_SLEEP );

    sx1276_2IoIrqInit( NULL );
        
    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        sx1276_2SetModem( RadioRegsInit[i].Modem );
        sx1276_2Write( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }

    sx1276_2SetModem( MODEM_FSK );

    sx1276_2.Settings.State = RF_IDLE;
}

RadioState_t sx1276_2GetStatus( void )
{
    return sx1276_2.Settings.State;
}

void sx1276_2SetChannel( uint32_t freq )
{
    sx1276_2.Settings.Channel = freq;
    freq = ( uint32_t )( freq / FREQ_STEP );
    sx1276_2Write( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    sx1276_2Write( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    sx1276_2Write( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

bool sx1276_2IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh )
{
    int16_t rssi = 0;
    
    sx1276_2SetModem( modem );

    sx1276_2SetChannel( freq );
    
    sx1276_2SetOpMode( RF_OPMODE_RECEIVER );

    mdelay( 1 );
    
    rssi = sx1276_2ReadRssi( modem );
    
    sx1276_2SetSleep( );
    
    if( rssi > rssiThresh )
    {
        return false;
    }
    return true;
}

uint32_t sx1276_2Random( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation 
     */
    // Set LoRa modem ON
    sx1276_2SetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    sx1276_2Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    sx1276_2SetOpMode( RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        mdelay( 1 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )sx1276_2Read( REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    sx1276_2SetSleep( );

    return rnd;
}

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( void )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = sx1276_2Read( REG_PACONFIG );
    initialFreq = ( ( ( uint32_t )sx1276_2Read( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )sx1276_2Read( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )sx1276_2Read( REG_FRFLSB ) ) ) * FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    sx1276_2Write( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    sx1276_2Write( REG_IMAGECAL, ( sx1276_2Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( sx1276_2Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    sx1276_2SetChannel( 868000000 );

    // Launch Rx chain calibration for HF band 
    sx1276_2Write( REG_IMAGECAL, ( sx1276_2Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( sx1276_2Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    sx1276_2Write( REG_PACONFIG, regPaConfigInitVal );
    sx1276_2SetChannel( initialFreq );
}

/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
static uint8_t GetFskBandwidthRegValue( uint32_t bandwidth )
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

void sx1276_2SetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    sx1276_2SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        {
            sx1276_2.Settings.Fsk.Bandwidth = bandwidth;
            sx1276_2.Settings.Fsk.Datarate = datarate;
            sx1276_2.Settings.Fsk.BandwidthAfc = bandwidthAfc;
            sx1276_2.Settings.Fsk.FixLen = fixLen;
            sx1276_2.Settings.Fsk.PayloadLen = payloadLen;
            sx1276_2.Settings.Fsk.CrcOn = crcOn;
            sx1276_2.Settings.Fsk.IqInverted = iqInverted;
            sx1276_2.Settings.Fsk.RxContinuous = rxContinuous;
            sx1276_2.Settings.Fsk.PreambleLen = preambleLen;
            #if 0
            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            sx1276_2Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            sx1276_2Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            sx1276_2Write( REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            sx1276_2Write( REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            sx1276_2Write( REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            sx1276_2Write( REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );
            
            if( fixLen == 1 )
            {
                sx1276_2Write( REG_PAYLOADLENGTH, payloadLen );
            }
            
            sx1276_2Write( REG_PACKETCONFIG1,
                         ( sx1276_2Read( REG_PACKETCONFIG1 ) & 
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
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
            sx1276_2.Settings.LoRa.Bandwidth = bandwidth;
            sx1276_2.Settings.LoRa.Datarate = datarate;
            sx1276_2.Settings.LoRa.Coderate = coderate;
            sx1276_2.Settings.LoRa.FixLen = fixLen;
            sx1276_2.Settings.LoRa.PayloadLen = payloadLen;
            sx1276_2.Settings.LoRa.CrcOn = crcOn;
            sx1276_2.Settings.LoRa.FreqHopOn = freqHopOn;
            sx1276_2.Settings.LoRa.HopPeriod = hopPeriod;
            sx1276_2.Settings.LoRa.IqInverted = iqInverted;
            sx1276_2.Settings.LoRa.RxContinuous = rxContinuous;

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
                sx1276_2.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                sx1276_2.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            sx1276_2Write( REG_LR_MODEMCONFIG1, 
                         ( sx1276_2Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) | 
                           fixLen );
                        
            sx1276_2Write( REG_LR_MODEMCONFIG2,
                         ( sx1276_2Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) |
                           ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            sx1276_2Write( REG_LR_MODEMCONFIG3, 
                         ( sx1276_2Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( sx1276_2.Settings.LoRa.LowDatarateOptimize << 3 ) );

            sx1276_2Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );
            
            sx1276_2Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            sx1276_2Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                sx1276_2Write( REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( sx1276_2.Settings.LoRa.FreqHopOn == true )
            {
                sx1276_2Write( REG_LR_PLLHOP, ( sx1276_2Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                sx1276_2Write( REG_LR_HOPPERIOD, sx1276_2.Settings.LoRa.HopPeriod );
            }

            if( ( bandwidth == 9 ) && ( RF_MID_BAND_THRESH ) )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth 
                sx1276_2Write( REG_LR_TEST36, 0x02 );
                sx1276_2Write( REG_LR_TEST3A, 0x64 );
            }
            else if( bandwidth == 9 )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                sx1276_2Write( REG_LR_TEST36, 0x02 );
                sx1276_2Write( REG_LR_TEST3A, 0x7F );
            }
            else
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                sx1276_2Write( REG_LR_TEST36, 0x03 );
            }
            
            if( datarate == 6 )
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE, 
                             ( sx1276_2Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                sx1276_2Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE,
                             ( sx1276_2Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                sx1276_2Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void sx1276_2SetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev, 
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn, 
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    sx1276_2SetModem( modem );
    
    paConfig = sx1276_2Read( REG_PACONFIG );
    paDac = sx1276_2Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | sx1276_2GetPaSelect( sx1276_2.Settings.Channel );
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
    sx1276_2Write( REG_PACONFIG, paConfig );
    sx1276_2Write( REG_PADAC, paDac );

    switch( modem )
    {
    case MODEM_FSK:
        {
            sx1276_2.Settings.Fsk.Power = power;
            sx1276_2.Settings.Fsk.Fdev = fdev;
            sx1276_2.Settings.Fsk.Bandwidth = bandwidth;
            sx1276_2.Settings.Fsk.Datarate = datarate;
            sx1276_2.Settings.Fsk.PreambleLen = preambleLen;
            sx1276_2.Settings.Fsk.FixLen = fixLen;
            sx1276_2.Settings.Fsk.CrcOn = crcOn;
            sx1276_2.Settings.Fsk.IqInverted = iqInverted;
            sx1276_2.Settings.Fsk.TxTimeout = timeout;
            #if 0
            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );
            sx1276_2Write( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            sx1276_2Write( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            sx1276_2Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            sx1276_2Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            sx1276_2Write( REG_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            sx1276_2Write( REG_PREAMBLELSB, preambleLen & 0xFF );

            sx1276_2Write( REG_PACKETCONFIG1,
                         ( sx1276_2Read( REG_PACKETCONFIG1 ) & 
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            #endif
        }
        break;
    case MODEM_LORA:
        {
            sx1276_2.Settings.LoRa.Power = power;
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
            sx1276_2.Settings.LoRa.Bandwidth = bandwidth;
            sx1276_2.Settings.LoRa.Datarate = datarate;
            sx1276_2.Settings.LoRa.Coderate = coderate;
            sx1276_2.Settings.LoRa.PreambleLen = preambleLen;
            sx1276_2.Settings.LoRa.FixLen = fixLen;
            sx1276_2.Settings.LoRa.FreqHopOn = freqHopOn;
            sx1276_2.Settings.LoRa.HopPeriod = hopPeriod;
            sx1276_2.Settings.LoRa.CrcOn = crcOn;
            sx1276_2.Settings.LoRa.IqInverted = iqInverted;
            sx1276_2.Settings.LoRa.TxTimeout = timeout;

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
                sx1276_2.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                sx1276_2.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            if( sx1276_2.Settings.LoRa.FreqHopOn == true )
            {
                sx1276_2Write( REG_LR_PLLHOP, ( sx1276_2Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                sx1276_2Write( REG_LR_HOPPERIOD, sx1276_2.Settings.LoRa.HopPeriod );
            }

            sx1276_2Write( REG_LR_MODEMCONFIG1, 
                         ( sx1276_2Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) | 
                           fixLen );

            sx1276_2Write( REG_LR_MODEMCONFIG2,
                         ( sx1276_2Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) );

            sx1276_2Write( REG_LR_MODEMCONFIG3, 
                         ( sx1276_2Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( sx1276_2.Settings.LoRa.LowDatarateOptimize << 3 ) );
        
            sx1276_2Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            sx1276_2Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );
            
            if( datarate == 6 )
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE, 
                             ( sx1276_2Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                sx1276_2Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE,
                             ( sx1276_2Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                sx1276_2Write( REG_LR_DETECTIONTHRESHOLD, 
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

uint32_t sx1276_2GetTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;
#if 0
    switch( modem )
    {
    case MODEM_FSK:
        {
            #if 0
            airTime = (long long)( ( 8 * ( sx1276_2.Settings.Fsk.PreambleLen +
                                     ( ( sx1276_2Read( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( sx1276_2.Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( sx1276_2Read( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( sx1276_2.Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                                     sx1276_2.Settings.Fsk.Datarate ) * 1e6 );
            #endif
        }
        break;
    case MODEM_LORA:
        {
            #if 0
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( sx1276_2.Settings.LoRa.Bandwidth )
            {
            //case 0: // 7.8 kHz
            //    bw = 78e2;
            //    break;
            //case 1: // 10.4 kHz
            //    bw = 104e2;
            //    break;
            //case 2: // 15.6 kHz
            //    bw = 156e2;
            //    break;
            //case 3: // 20.8 kHz
            //    bw = 208e2;
            //    break;
            //case 4: // 31.2 kHz
            //    bw = 312e2;
            //    break;
            //case 5: // 41.4 kHz
            //    bw = 414e2;
            //    break;
            //case 6: // 62.5 kHz
            //    bw = 625e2;
            //    break;
            case 7: // 125 kHz
                bw = 125e3;
                break;
            case 8: // 250 kHz
                bw = 250e3;
                break;
            case 9: // 500 kHz
                bw = 500e3;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = (double)bw / ( 1 << sx1276_2.Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( sx1276_2.Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = (long long)( ( 8 * pktLen - 4 * sx1276_2.Settings.LoRa.Datarate +
                                 28 + 16 * sx1276_2.Settings.LoRa.CrcOn -
                                 ( sx1276_2.Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * sx1276_2.Settings.LoRa.Datarate -
                                 ( ( sx1276_2.Settings.LoRa.LowDatarateOptimize > 0 ) ? 8 : 0 ) ) ) *
                                 ( sx1276_2.Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air 
            double tOnAir = tPreamble + tPayload;
            // return us secs
            airTime = (long long)( tOnAir * 1e6 + 0.999 );
            #endif
        }
        break;
    }
#endif
    return airTime;
}

void sx1276_2Send( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        {
            sx1276_2.Settings.FskPacketHandler.NbBytes = 0;
            sx1276_2.Settings.FskPacketHandler.Size = size;

            if( sx1276_2.Settings.Fsk.FixLen == false )
            {
                sx1276_2WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                sx1276_2Write( REG_PAYLOADLENGTH, size );
            }            
            
            if( ( size > 0 ) && ( size <= 64 ) )
            {
                sx1276_2.Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                sx1276_2.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            sx1276_2WriteFifo( buffer, sx1276_2.Settings.FskPacketHandler.ChunkSize );
            sx1276_2.Settings.FskPacketHandler.NbBytes += sx1276_2.Settings.FskPacketHandler.ChunkSize;
            txTimeout = sx1276_2.Settings.Fsk.TxTimeout;
        }
        break;
    case MODEM_LORA:
        {
            if( sx1276_2.Settings.LoRa.IqInverted == true )
            {
                sx1276_2Write( REG_LR_INVERTIQ, ( ( sx1276_2Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                sx1276_2Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                sx1276_2Write( REG_LR_INVERTIQ, ( ( sx1276_2Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                sx1276_2Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }      
        
            sx1276_2.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            sx1276_2Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx            
            sx1276_2Write( REG_LR_FIFOTXBASEADDR, 0 );
            sx1276_2Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( sx1276_2Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                sx1276_2SetStby( );
                mdelay( 1 );
            }
            // Write payload buffer
            sx1276_2WriteFifo( buffer, size );
            txTimeout = sx1276_2.Settings.LoRa.TxTimeout;
        }
        break;
    }

    sx1276_2SetTx( txTimeout );
}

void sx1276_2SetSleep( void )
{
    del_timer( &RxTimeoutTimer );
    del_timer( &TxTimeoutTimer );

    sx1276_2SetOpMode( RF_OPMODE_SLEEP );
    sx1276_2.Settings.State = RF_IDLE;
}

void sx1276_2SetStby( void )
{
    del_timer( &RxTimeoutTimer );
    del_timer( &TxTimeoutTimer );

    sx1276_2SetOpMode( RF_OPMODE_STANDBY );
    sx1276_2.Settings.State = RF_IDLE;
}

void sx1276_2SetRx( uint32_t timeout )
{
    bool rxContinuous = false;
    uint32_t time;
    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        {
            rxContinuous = sx1276_2.Settings.Fsk.RxContinuous;
            
            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );
            
            sx1276_2Write( REG_DIOMAPPING2, ( sx1276_2Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) | 
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );
            
            sx1276_2.Settings.FskPacketHandler.FifoThresh = sx1276_2Read( REG_FIFOTHRESH ) & 0x3F;
            
            sx1276_2.Settings.FskPacketHandler.PreambleDetected = false;
            sx1276_2.Settings.FskPacketHandler.SyncWordDetected = false;
            sx1276_2.Settings.FskPacketHandler.NbBytes = 0;
            sx1276_2.Settings.FskPacketHandler.Size = 0;
        }
        break;
    case MODEM_LORA:
        {
            if( sx1276_2.Settings.LoRa.IqInverted == true )
            {
                sx1276_2Write( REG_LR_INVERTIQ, ( ( sx1276_2Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                sx1276_2Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                sx1276_2Write( REG_LR_INVERTIQ, ( ( sx1276_2Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                sx1276_2Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }         

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( sx1276_2.Settings.LoRa.Bandwidth < 9 )
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE, sx1276_2Read( REG_LR_DETECTOPTIMIZE ) & 0x7F );
                sx1276_2Write( REG_LR_TEST30, 0x00 );
                switch( sx1276_2.Settings.LoRa.Bandwidth )
                {
                #if 0
                case 0: // 7.8 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x48 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 7.81e3 );
                    break;
                case 1: // 10.4 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x44 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 10.42e3 );
                    break;
                case 2: // 15.6 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x44 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 15.62e3 );
                    break;
                case 3: // 20.8 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x44 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 20.83e3 );
                    break;
                case 4: // 31.2 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x44 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 31.25e3 );
                    break;
                case 5: // 41.4 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x44 );
                    sx1276_2SetChannel(sx1276_2.Settings.Channel + 41.67e3 );
                    break;
                #endif
                case 6: // 62.5 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x40 );
                    break;
                case 7: // 125 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x40 );
                    break;
                case 8: // 250 kHz
                    sx1276_2Write( REG_LR_TEST2F, 0x40 );
                    break;
                }
            }
            else
            {
                sx1276_2Write( REG_LR_DETECTOPTIMIZE, sx1276_2Read( REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }

            rxContinuous = sx1276_2.Settings.LoRa.RxContinuous;
            
            if( sx1276_2.Settings.LoRa.FreqHopOn == true )
            {
                sx1276_2Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=RxDone, DIO2=FhssChangeChannel
                sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                sx1276_2Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=RxDone
                sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            sx1276_2Write( REG_LR_FIFORXBASEADDR, 0 );
            sx1276_2Write( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    memset( RxBuffer, 0, ( size_t )RX_BUFFER_SIZE );

    sx1276_2.Settings.State = RF_RX_RUNNING;
    if( timeout != 0 )
    {
        del_timer(&RxTimeoutTimer);
        RxTimeoutTimer.expires = jiffies + HZ;        //定时时间
        RxTimeoutTimer.function = sx1276_2OnTimeoutIrq;
        add_timer(&RxTimeoutTimer);        //注册定时器
    }

    if( sx1276_2.Settings.Modem == MODEM_FSK )
    {
        #if 0
        sx1276_2SetOpMode( RF_OPMODE_RECEIVER );
        
        if( rxContinuous == false )
        {
            time = ( 8.0 * ( sx1276_2.Settings.Fsk.PreambleLen +
                                                         ( ( sx1276_2Read( REG_SYNCCONFIG ) &
                                                            ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
                                                         1.0 ) + 10.0 ) /
                                                        ( double )sx1276_2.Settings.Fsk.Datarate ) * 1e6;
            del_timer(&RxTimeoutSyncWord);
            RxTimeoutSyncWord.expires = jiffies + HZ / (time * 10);        //定时时间
            RxTimeoutSyncWord.function = sx1276_2OnTimeoutIrq;
            add_timer(&RxTimeoutSyncWord);        //注册定时器
        }
        #endif
    }
    else
    {
        if( rxContinuous == true )
        {
            sx1276_2SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            sx1276_2SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void sx1276_2SetTx( uint32_t timeout )
{
    del_timer(&TxTimeoutTimer);
    TxTimeoutTimer.expires = jiffies + HZ / timeout;        //定时时间
    TxTimeoutTimer.function = sx1276_2OnTimeoutIrq;
    //add_timer(&TxTimeoutTimer);        //注册定时器

    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        {
            // DIO0=PacketSent
            // DIO1=FifoLevel
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) );

            sx1276_2Write( REG_DIOMAPPING2, ( sx1276_2Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            sx1276_2.Settings.FskPacketHandler.FifoThresh = sx1276_2Read( REG_FIFOTHRESH ) & 0x3F;
        }
        break;
    case MODEM_LORA:
        {
            if( sx1276_2.Settings.LoRa.FreqHopOn == true )
            {
                sx1276_2Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );
                                              
                // DIO0=TxDone, DIO2=FhssChangeChannel
                sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                sx1276_2Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }

    sx1276_2.Settings.State = RF_TX_RUNNING;
    add_timer(&TxTimeoutTimer);        //注册定时器
    sx1276_2SetOpMode( RF_OPMODE_TRANSMITTER );
}

void sx1276_2StartCad( void )
{
    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        {
           
        }
        break;
    case MODEM_LORA:
        {
            sx1276_2Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED 
                                        );
                                          
            // DIO3=CADDone
            sx1276_2Write( REG_DIOMAPPING1, ( sx1276_2Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            
            sx1276_2.Settings.State = RF_CAD;
            sx1276_2SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

int16_t sx1276_2ReadRssi( RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( sx1276_2Read( REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( sx1276_2.Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + sx1276_2Read( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + sx1276_2Read( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}

void sx1276_2Reset( void )
{    
    //gpio_set_value(SX1278_1_RST_PIN,0);
    gpio_set_value(SX1278_2_RST_PIN,0);
    udelay(1000);
    //gpio_set_value(SX1278_1_RST_PIN,1);
    gpio_set_value(SX1278_2_RST_PIN,1);
    udelay(6000);
}

void sx1276_2SetOpMode( uint8_t opMode )
{
    static uint8_t opModePrev = RF_OPMODE_STANDBY;

    if( opMode != opModePrev )
    {
        opModePrev = opMode;
        if( opMode == RF_OPMODE_SLEEP )
        {
            sx1276_2SetAntSwLowPower( true );
        }
        else
        {
            sx1276_2SetAntSwLowPower( false );
            if( opMode == RF_OPMODE_TRANSMITTER )
            {
                 sx1276_2SetAntSw( 1 );
            }
            else
            {
                 sx1276_2SetAntSw( 0 );
            }
        }
        sx1276_2Write( REG_OPMODE, ( sx1276_2Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
    }
}

void sx1276_2SetModem( RadioModems_t modem )
{
    if( sx1276_2.Spi == NULL )
    {
        while( 1 );
    }
    if( sx1276_2.Settings.Modem == modem )
    {
        return;
    }

    sx1276_2.Settings.Modem = modem;
    switch( sx1276_2.Settings.Modem )
    {
    default:
    case MODEM_FSK:
        sx1276_2SetOpMode( RF_OPMODE_SLEEP );
        sx1276_2Write( REG_OPMODE, ( sx1276_2Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );
    
        sx1276_2Write( REG_DIOMAPPING1, 0x00 );
        sx1276_2Write( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
        break;
    case MODEM_LORA:
        sx1276_2SetOpMode( RF_OPMODE_SLEEP );
        sx1276_2Write( REG_OPMODE, ( sx1276_2Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        sx1276_2Write( REG_DIOMAPPING1, 0x00 );
        sx1276_2Write( REG_DIOMAPPING2, 0x00 );
        break;
    }
}

void sx1276_2Write( uint8_t addr, uint8_t data )
{
    sx1276_2WriteBuffer( addr, &data, 1 );
}

uint8_t sx1276_2Read( uint8_t addr )
{
    uint8_t data;
    sx1276_2ReadBuffer( addr, &data, 1 );
    return data;
}

void sx1276_2WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t *data = kmalloc(size + 1,GFP_KERNEL);
    data[0] = addr | 0x80;
    memcpy(data + 1,buffer,size);
    //spin_lock_bh(&spi_lock);
    spi_write(sx1276_2.Spi,data,size + 1);
    //spin_unlock_bh(&spi_lock);
    kfree(data);
}

void sx1276_2ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    addr = addr & 0x7F;
    //spin_lock_bh(&spi_lock);
    spi_write_then_read(sx1276_2.Spi,&addr,1,buffer,size);
    //spin_unlock_bh(&spi_lock);
}

void sx1276_2WriteFifo( uint8_t *buffer, uint8_t size )
{
    sx1276_2WriteBuffer( 0, buffer, size );
}

void sx1276_2ReadFifo( uint8_t *buffer, uint8_t size )
{
    sx1276_2ReadBuffer( 0, buffer, size );
}

void sx1276_2SetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    sx1276_2SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( sx1276_2.Settings.Fsk.FixLen == false )
        {
            sx1276_2Write( REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        sx1276_2Write( REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}

void sx1276_2OnTimeoutIrq( unsigned long arg )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    switch( sx1276_2.Settings.State )
    {
    case RF_RX_RUNNING:
        if( sx1276_2.Settings.Modem == MODEM_FSK )
        {
            sx1276_2.Settings.FskPacketHandler.PreambleDetected = false;
            sx1276_2.Settings.FskPacketHandler.SyncWordDetected = false;
            sx1276_2.Settings.FskPacketHandler.NbBytes = 0;
            sx1276_2.Settings.FskPacketHandler.Size = 0;

            // Clear Irqs
            sx1276_2Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | 
                                        RF_IRQFLAGS1_PREAMBLEDETECT |
                                        RF_IRQFLAGS1_SYNCADDRESSMATCH );
            sx1276_2Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

            if( sx1276_2.Settings.Fsk.RxContinuous == true )
            {
                // Continuous mode restart Rx chain
                sx1276_2Write( REG_RXCONFIG, sx1276_2Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
            }
            else
            {
                sx1276_2.Settings.State = RF_IDLE;
                del_timer(&RxTimeoutSyncWord);
            }
        }
        if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
        {
            RadioEvents->RxTimeout( );
        }
        break;
    case RF_TX_RUNNING:
        sx1276_2.Settings.State = RF_IDLE;
        if( ( RadioEvents != NULL ) && ( RadioEvents->TxTimeout != NULL ) )
        {
            RadioEvents->TxTimeout( );
        }
        break;
    default:
        break;
    }
}

void sx1276_2OnDio0Irq( unsigned long param)
{
    volatile uint8_t irqFlags = 0;

    switch( sx1276_2.Settings.State )
    {                
        case RF_RX_RUNNING:
            //TimerStop( &RxTimeoutTimer );
            // RxDone interrupt
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_FSK:
                if( sx1276_2.Settings.Fsk.CrcOn == true )
                {
                    irqFlags = sx1276_2Read( REG_IRQFLAGS2 );
                    if( ( irqFlags & RF_IRQFLAGS2_CRCOK ) != RF_IRQFLAGS2_CRCOK )
                    {
                        // Clear Irqs
                        sx1276_2Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI | 
                                                    RF_IRQFLAGS1_PREAMBLEDETECT |
                                                    RF_IRQFLAGS1_SYNCADDRESSMATCH );
                        sx1276_2Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

                        if( sx1276_2.Settings.Fsk.RxContinuous == false )
                        {
                            sx1276_2.Settings.State = RF_IDLE;
                            add_timer( &RxTimeoutSyncWord );
                        }
                        else
                        {
                            // Continuous mode restart Rx chain
                            sx1276_2Write( REG_RXCONFIG, sx1276_2Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                        }
                        del_timer( &RxTimeoutTimer );

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError( ); 
                        }
                        sx1276_2.Settings.FskPacketHandler.PreambleDetected = false;
                        sx1276_2.Settings.FskPacketHandler.SyncWordDetected = false;
                        sx1276_2.Settings.FskPacketHandler.NbBytes = 0;
                        sx1276_2.Settings.FskPacketHandler.Size = 0;
                        break;
                    }
                }
                
                // Read received packet size
                if( ( sx1276_2.Settings.FskPacketHandler.Size == 0 ) && ( sx1276_2.Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( sx1276_2.Settings.Fsk.FixLen == false )
                    {
                        sx1276_2ReadFifo( ( uint8_t* )&sx1276_2.Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        sx1276_2.Settings.FskPacketHandler.Size = sx1276_2Read( REG_PAYLOADLENGTH );
                    }
                    sx1276_2ReadFifo( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes, sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += ( sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                }
                else
                {
                    sx1276_2ReadFifo( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes, sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += ( sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                }

                if( sx1276_2.Settings.Fsk.RxContinuous == false )
                {
                    sx1276_2.Settings.State = RF_IDLE;
                    add_timer( &RxTimeoutSyncWord );
                }
                else
                {
                    // Continuous mode restart Rx chain
                    sx1276_2Write( REG_RXCONFIG, sx1276_2Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                }
                del_timer( &RxTimeoutTimer );

                if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                {
                    RadioEvents->RxDone( RxBuffer, sx1276_2.Settings.FskPacketHandler.Size, sx1276_2.Settings.FskPacketHandler.RssiValue, 0 ); 
                } 
                sx1276_2.Settings.FskPacketHandler.PreambleDetected = false;
                sx1276_2.Settings.FskPacketHandler.SyncWordDetected = false;
                sx1276_2.Settings.FskPacketHandler.NbBytes = 0;
                sx1276_2.Settings.FskPacketHandler.Size = 0;
                break;
            case MODEM_LORA:
                {
                    int8_t snr = 0;

                    // Clear Irq
                    sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );

                    irqFlags = sx1276_2Read( REG_LR_IRQFLAGS );
                    if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
                    {
                        // Clear Irq
                        sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );

                        if( sx1276_2.Settings.LoRa.RxContinuous == false )
                        {
                            sx1276_2.Settings.State = RF_IDLE;
                        }
                        del_timer( &RxTimeoutTimer );

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError( ); 
                        }
                        break;
                    }

                    sx1276_2.Settings.LoRaPacketHandler.SnrValue = sx1276_2Read( REG_LR_PKTSNRVALUE );
                    if( sx1276_2.Settings.LoRaPacketHandler.SnrValue & 0x80 ) // The SNR sign bit is 1
                    {
                        // Invert and divide by 4
                        snr = ( ( ~sx1276_2.Settings.LoRaPacketHandler.SnrValue + 1 ) & 0xFF ) >> 2;
                        snr = -snr;
                    }
                    else
                    {
                        // Divide by 4
                        snr = ( sx1276_2.Settings.LoRaPacketHandler.SnrValue & 0xFF ) >> 2;
                    }

                    int16_t rssi = (int16_t)sx1276_2Read( REG_LR_PKTRSSIVALUE );
                    if( snr < 0 )
                    {
                        if( sx1276_2.Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            sx1276_2.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                        else
                        {
                            sx1276_2.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                    }
                    else
                    {    
                        if( sx1276_2.Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            sx1276_2.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 );
                        }
                        else
                        {
                            sx1276_2.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 );
                        }
                    }

                    sx1276_2.Settings.LoRaPacketHandler.Size = sx1276_2Read( REG_LR_RXNBBYTES );
                    sx1276_2ReadFifo( RxBuffer, sx1276_2.Settings.LoRaPacketHandler.Size );
                
                    if( sx1276_2.Settings.LoRa.RxContinuous == false )
                    {
                        sx1276_2.Settings.State = RF_IDLE;
                    }
                    del_timer( &RxTimeoutTimer );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                    {
                        RadioEvents->RxDone( RxBuffer, sx1276_2.Settings.LoRaPacketHandler.Size, sx1276_2.Settings.LoRaPacketHandler.RssiValue, sx1276_2.Settings.LoRaPacketHandler.SnrValue );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            del_timer( &TxTimeoutTimer );
            // TxDone interrupt
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_LORA:
                // Clear Irq
                sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
                // Intentional fall through
            case MODEM_FSK:
            default:
                sx1276_2.Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->TxDone != NULL ) )
                {
                    RadioEvents->TxDone( );
                } 
                break;
            }
            break;
        default:
            break;
    }
}

void sx1276_2OnDio1Irq( unsigned long param)
{
    switch( sx1276_2.Settings.State )
    {                
        case RF_RX_RUNNING:
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if( ( sx1276_2.Settings.FskPacketHandler.Size == 0 ) && ( sx1276_2.Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( sx1276_2.Settings.Fsk.FixLen == false )
                    {
                        sx1276_2ReadFifo( ( uint8_t* )&sx1276_2.Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        sx1276_2.Settings.FskPacketHandler.Size = sx1276_2Read( REG_PAYLOADLENGTH );
                    }
                }

                if( ( sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes ) > sx1276_2.Settings.FskPacketHandler.FifoThresh )
                {
                    sx1276_2ReadFifo( ( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes ), sx1276_2.Settings.FskPacketHandler.FifoThresh );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += sx1276_2.Settings.FskPacketHandler.FifoThresh;
                }
                else
                {
                    sx1276_2ReadFifo( ( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes ), sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += ( sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                }
                break;
            case MODEM_LORA:
                // Sync time out
                del_timer( &RxTimeoutTimer );
                sx1276_2.Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
                {
                    RadioEvents->RxTimeout( );
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoLevel interrupt
                if( ( sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes ) > sx1276_2.Settings.FskPacketHandler.ChunkSize )
                {
                    sx1276_2WriteFifo( ( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes ), sx1276_2.Settings.FskPacketHandler.ChunkSize );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += sx1276_2.Settings.FskPacketHandler.ChunkSize;
                }
                else 
                {
                    // Write the last chunk of data
                    sx1276_2WriteFifo( RxBuffer + sx1276_2.Settings.FskPacketHandler.NbBytes, sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes );
                    sx1276_2.Settings.FskPacketHandler.NbBytes += sx1276_2.Settings.FskPacketHandler.Size - sx1276_2.Settings.FskPacketHandler.NbBytes;
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
}

void sx1276_2OnDio2Irq( unsigned long param)
{
    switch( sx1276_2.Settings.State )
    {                
        case RF_RX_RUNNING:
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_FSK:
                if( ( sx1276_2.Settings.FskPacketHandler.PreambleDetected == true ) && ( sx1276_2.Settings.FskPacketHandler.SyncWordDetected == false ) )
                {
                    #if 0
                    del_timer( &RxTimeoutSyncWord );
                    
                    sx1276_2.Settings.FskPacketHandler.SyncWordDetected = true;
                
                    sx1276_2.Settings.FskPacketHandler.RssiValue = -( sx1276_2Read( REG_RSSIVALUE ) >> 1 );

                    sx1276_2.Settings.FskPacketHandler.AfcValue = ( int32_t )( double )( ( ( uint16_t )sx1276_2Read( REG_AFCMSB ) << 8 ) |
                                                                           ( uint16_t )sx1276_2Read( REG_AFCLSB ) ) *
                                                                           ( double )FREQ_STEP;
                    sx1276_2.Settings.FskPacketHandler.RxGain = ( sx1276_2Read( REG_LNA ) >> 5 ) & 0x07;
                    #endif
                }
                break;
            case MODEM_LORA:
                if( sx1276_2.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
                    
                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( sx1276_2Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( sx1276_2.Settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if( sx1276_2.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
                    
                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( sx1276_2Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
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

void sx1276_2OnDio3Irq( unsigned long param)
{
    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        if( ( sx1276_2Read( REG_LR_IRQFLAGS ) & RFLR_IRQFLAGS_CADDETECTED ) == RFLR_IRQFLAGS_CADDETECTED )
        {
            // Clear Irq
            sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone( true );
            }
        }
        else
        {        
            // Clear Irq
            sx1276_2Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone( false );
            }
        }
        break;
    default:
        break;
    }
}

void sx1276_2OnDio4Irq( unsigned long param)
{
    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        {
            if( sx1276_2.Settings.FskPacketHandler.PreambleDetected == false )
            {
                sx1276_2.Settings.FskPacketHandler.PreambleDetected = true;
            }    
        }
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void sx1276_2OnDio5Irq( unsigned long param)
{
    switch( sx1276_2.Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}
