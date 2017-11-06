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
#include "radio.h"
#include "sx1276.h"
#include "utilities.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
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
 * Private global constants
 */

/*!
 * Radio hardware registers initialization
 *
 * \remark RADIO_INIT_REGISTERS_VALUE is defined in sx1276-board.h file
 */
//const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

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
 * Reception buffer
 */
static uint8_t RxTxBuffer[RX_BUFFER_SIZE];

/*
 * Public global variables
 */

/*!
 * Radio hardware and global parameters
 */
SX1276_t SX1276[2];

/*!
 * Hardware DIO IRQ callback initialization
 */
/*DioIrqHandler *DioIrq[] = { SX1276OnDio0Irq, SX1276OnDio1Irq,
                            SX1276OnDio2Irq, SX1276OnDio3Irq,
                            SX1276OnDio4Irq, NULL };
*/

#define LORADEV_IOC_MAGIC  'r'

#define LORADEV_IOCPRINT   _IO(LORADEV_IOC_MAGIC, 0)  //没参数
#define LORADEV_IOCGETDATA _IOR(LORADEV_IOC_MAGIC, 1, int)  //读
#define LORADEV_IOCSETDATA _IOW(LORADEV_IOC_MAGIC, 2, int)  //写
#define LORADEV_RADIO_INIT   _IOW(LORADEV_IOC_MAGIC, 3, int)  //没参数
#define LORADEV_RADIO_STATE _IOW(LORADEV_IOC_MAGIC, 4, int)  //读
#define LORADEV_RADIO_CHANNEL _IOW(LORADEV_IOC_MAGIC, 5, int)  //写
#define LORADEV_RADIO_SET_PUBLIC _IOW(LORADEV_IOC_MAGIC, 6, int)  //写
#define LORADEV_RADIO_SET_MODEM _IOW(LORADEV_IOC_MAGIC, 7, int)  //写
#define LORADEV_RADIO_READ_REG _IOWR(LORADEV_IOC_MAGIC, 8, int)  //写
#define LORADEV_RADIO_SET_TXCFG _IOW(LORADEV_IOC_MAGIC, 9, int)
#define LORADEV_RADIO_SET_RXCFG _IOW(LORADEV_IOC_MAGIC, 10, int)
#define LORADEV_IOC_MAXNR 11

/*
 * Radio driver functions implementation
 */

void SX1276Init( int chip,RadioEvents_t *events )
{
	printf("%s,%d,cmd = %d\r\n",__func__,__LINE__,LORADEV_RADIO_INIT);
	//ioctl(fd, LORADEV_RADIO_INIT, NULL);
}

RadioState_t SX1276GetStatus( int chip )
{
	return RF_IDLE;
}

void SX1276SetChannel( int chip,uint32_t freq )
{
    SX1276[chip].Settings.Channel = freq;
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
	freq = freq | (chip << 31);
    ioctl(fd_cdev, LORADEV_RADIO_CHANNEL, &freq);
    printf("%s,%d,freq = %d\r\n",__func__,__LINE__,freq);
}

bool SX1276IsChannelFree( int chip,RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{

}

uint32_t SX1276Random( int chip )
{

}

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( int chip )
{

}

/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
static uint8_t GetFskBandwidthRegValue( int chip,uint32_t bandwidth )
{

}

void SX1276SetRxConfig( int chip,RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
	printf("%s,%d\r\n",__func__,__LINE__);
	lora_proc_data_t lora_proc_data;
	memset(&lora_proc_data,0,sizeof(lora_proc_data_t));
	memcpy(lora_proc_data.name,"RegionCN470RxConfig",strlen("RegionCN470RxConfig"));
	lora_proc_data.proc_value.stRxconfig.chip = chip;
	lora_proc_data.proc_value.stRxconfig.modem = modem;
	lora_proc_data.proc_value.stRxconfig.bandwidth = bandwidth;
	lora_proc_data.proc_value.stRxconfig.datarate = datarate;
	lora_proc_data.proc_value.stRxconfig.coderate = coderate;
	lora_proc_data.proc_value.stRxconfig.bandwidthAfc = bandwidthAfc;
	lora_proc_data.proc_value.stRxconfig.preambleLen = preambleLen;
	lora_proc_data.proc_value.stRxconfig.symbTimeout = symbTimeout;
	lora_proc_data.proc_value.stRxconfig.fixLen = fixLen;
	lora_proc_data.proc_value.stRxconfig.payloadLen = payloadLen;
	lora_proc_data.proc_value.stRxconfig.crcOn = crcOn;
	lora_proc_data.proc_value.stRxconfig.freqHopOn = freqHopOn;
	lora_proc_data.proc_value.stRxconfig.hopPeriod = hopPeriod;
	lora_proc_data.proc_value.stRxconfig.iqInverted = iqInverted;
	lora_proc_data.proc_value.stRxconfig.rxContinuous = rxContinuous;
	
	write(fd_proc,&lora_proc_data,sizeof(lora_proc_data_t));
	//ioctl(fd, LORADEV_RADIO_SET_RXCFG, NULL);
}

void SX1276SetTxConfig( int chip,RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
	printf("%s,%d\r\n",__func__,__LINE__);
	lora_proc_data_t lora_proc_data;
	memset(&lora_proc_data,0,sizeof(lora_proc_data_t));
	memcpy(lora_proc_data.name,"RegionCN470TxConfig",strlen("RegionCN470TxConfig"));
	lora_proc_data.proc_value.stTxconfig.chip = chip;
	lora_proc_data.proc_value.stTxconfig.modem = modem;
	lora_proc_data.proc_value.stTxconfig.power = power;
	lora_proc_data.proc_value.stTxconfig.fdev = fdev;
	lora_proc_data.proc_value.stTxconfig.bandwidth = bandwidth;
	lora_proc_data.proc_value.stTxconfig.datarate = datarate;
	lora_proc_data.proc_value.stTxconfig.coderate = coderate;
	lora_proc_data.proc_value.stTxconfig.preambleLen = preambleLen;
	lora_proc_data.proc_value.stTxconfig.fixLen = fixLen;
	lora_proc_data.proc_value.stTxconfig.crcOn = crcOn;
	lora_proc_data.proc_value.stTxconfig.freqHopOn = freqHopOn;
	lora_proc_data.proc_value.stTxconfig.hopPeriod = hopPeriod;
	lora_proc_data.proc_value.stTxconfig.iqInverted = iqInverted;
	lora_proc_data.proc_value.stTxconfig.timeout = timeout;

	write(fd_proc,&lora_proc_data,sizeof(lora_proc_data_t));
}

bool SX1276CheckRfFrequency( int chip,uint32_t frequency )
{
	// Implement check. Currently all frequencies are supported
	return true;
}

uint32_t SX1276GetTimeOnAir( int chip,RadioModems_t modem, uint8_t pktLen )
{

}

void SX1276Send( int chip,uint8_t *buffer, uint8_t size )
{

}

void SX1276SetSleep( int chip )
{

}

void SX1276SetStby( int chip )
{

}

void SX1276SetRx( int chip,uint32_t timeout )
{

}

void SX1276SetTx( int chip,uint32_t timeout )
{

}

void SX1276StartCad( int chip )
{

}

void SX1276SetTxContinuousWave( int chip,uint32_t freq, int8_t power, uint16_t time )
{

}

int16_t SX1276ReadRssi( int chip,RadioModems_t modem )
{

}

void SX1276Reset( int chip )
{

}

void SX1276SetOpMode( int chip,uint8_t opMode )
{
}

void SX1276SetModem( int chip,RadioModems_t modem )
{
	printf("%s,%d\r\n",__func__,__LINE__);
	//ioctl(fd, LORADEV_RADIO_SET_MODEM, &modem);
}

void SX1276Write( int chip,uint8_t addr, uint8_t data )
{
}

uint8_t SX1276Read( int chip,uint8_t addr )
{
}

void SX1276WriteBuffer( int chip,uint8_t addr, uint8_t *buffer, uint8_t size )
{
}

void SX1276ReadBuffer( int chip,uint8_t addr, uint8_t *buffer, uint8_t size )
{
}

void SX1276WriteFifo( int chip,uint8_t *buffer, uint8_t size )
{
}

void SX1276ReadFifo( int chip,uint8_t *buffer, uint8_t size )
{
}

void SX1276SetMaxPayloadLength( int chip,RadioModems_t modem, uint8_t max )
{
}

void SX1276SetPublicNetwork( int chip,bool enable )
{
	int ioarg = enable;
	ioarg = ioarg | (chip << 31);
	ioctl(fd_cdev, LORADEV_RADIO_SET_PUBLIC, &ioarg);
	printf("%s,%d,freq = 0x%04x\r\n",__func__,__LINE__,ioarg);
}

void SX1276OnTimeoutIrq( unsigned long chip )
{
}

