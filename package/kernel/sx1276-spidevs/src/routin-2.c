#include "radio.h"
#include "routin-2.h"
#include <linux/sched.h>  //wake_up_process()
#include <linux/kthread.h>//kthread_create()、kthread_run()
#include <linux/err.h>             //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/gpio.h>
#include "pinmap.h"

#define USE_BAND_433
#define USE_MODEM_LORA

#if defined( USE_BAND_433 )

#define RF_FREQUENCY                                434000000 // Hz

#elif defined( USE_BAND_780 )

#define RF_FREQUENCY                                780000000 // Hz

#elif defined( USE_BAND_868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( USE_BAND_915 )

#define RF_FREQUENCY                                915000000 // Hz

#else
    #error "Please define a frequency band in the compiler options."
#endif

#define TX_OUTPUT_POWER                             20        // dBm

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    25e3      // Hz
#define FSK_DATARATE                                50e3      // bps
#define FSK_BANDWIDTH                               50e3      // Hz
#define FSK_AFC_BANDWIDTH                           83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here

static const uint8_t PingMsg[] = "PING";
static const uint8_t PongMsg[] = "PONG";

static uint16_t BufferSize = BUFFER_SIZE;
static uint8_t Buffer[BUFFER_SIZE];

static States_t State = LOWPOWER;

static int8_t RssiValue = 0;
static int8_t SnrValue = 0;

/*!
 * Radio_2 events function pointer
 */
static RadioEvents_t Radio_2Events;

/*!
 * \brief Function to be executed on Radio_2 Tx Done event
 */
static void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio_2 Rx Done event
 */
static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio_2 Tx Timeout event
 */
static void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio_2 Rx Timeout event
 */
static void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio_2 Rx Error event
 */
static void OnRxError( void );

static void OnTxDone( void )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    Radio_2.Sleep( );
    State = TX;
}

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    Radio_2.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
}

static void OnTxTimeout( void )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    Radio_2.Sleep( );
    State = TX_TIMEOUT;
}

static void OnRxTimeout( void )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    Radio_2.Sleep( );
    State = RX_TIMEOUT;
}

static void OnRxError( void )
{
    printk(KERN_DEBUG "%s\r\n",__func__);
    Radio_2.Sleep( );
    State = RX_ERROR;
}

int Radio_2_routin(void *data){
	bool isMaster = true;
    uint8_t i;
    int err;

	// Radio_2 initialization
 	Radio_2Events.TxDone = OnTxDone;
	Radio_2Events.RxDone = OnRxDone;
	Radio_2Events.TxTimeout = OnTxTimeout;
	Radio_2Events.RxTimeout = OnRxTimeout;
	Radio_2Events.RxError = OnRxError;
	Radio_2.Init( &Radio_2Events );
	Radio_2.SetChannel( RF_FREQUENCY );
#if defined( USE_MODEM_LORA )

	Radio_2.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
									LORA_SPREADING_FACTOR, LORA_CODINGRATE,
									LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
									true, 0, 0, LORA_IQ_INVERSION_ON, 1000 );
	Radio_2.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
									LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
									LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
									0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
#elif defined( USE_MODEM_FSK )

	Radio_2.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
									FSK_DATARATE, 0,
									FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
									true, 0, 0, 0, 3000000 );

	Radio_2.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
									0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
									0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
 									0, 0,false, true );

#else
	#error "Please define a frequency band in the compiler options."
#endif
    msleep(600);
	Radio_2.Rx( RX_TIMEOUT_VALUE );
	while(1)
	{
		switch( State )
        {
        case RX:
            if( isMaster == true )
            {
                if( BufferSize > 0 )
                {
                    if( strncmp( ( const char* )Buffer, ( const char* )PongMsg, 4 ) == 0 )
                    {
                        // Indicates on a LED that the received frame is a PONG
                        printk("received a message 'pong'\r\n");

                        // Send the next PING frame            
                        Buffer[0] = 'P';
                        Buffer[1] = 'I';
                        Buffer[2] = 'N';
                        Buffer[3] = 'G';
                        // We fill the buffer with numbers for the payload 
                        for( i = 4; i < BufferSize; i++ )
                        {
                            Buffer[i] = i - 4;
                        }
                        udelay( 1000 ); 
                        Radio_2.Send( Buffer, BufferSize );
                    }
                    else if( strncmp( ( const char* )Buffer, ( const char* )PingMsg, 4 ) == 0 )
                    { // A master already exists then become a slave
                        isMaster = false;
                        printk("received a message 'ping'\r\n");
                        Radio_2.Rx( RX_TIMEOUT_VALUE );
                    }
                    else // valid reception but neither a PING or a PONG message
                    {    // Set device as master ans start again
                        isMaster = true;
                        Radio_2.Rx( RX_TIMEOUT_VALUE );
                    }
                }
            }
            else
            {
                if( BufferSize > 0 )
                {
                    if( strncmp( ( const char* )Buffer, ( const char* )PingMsg, 4 ) == 0 )
                    {
                        // Indicates on a LED that the received frame is a PING
                        printk("received a message 'ping'\r\n");

                        // Send the reply to the PONG string
                        Buffer[0] = 'P';
                        Buffer[1] = 'O';
                        Buffer[2] = 'N';
                        Buffer[3] = 'G';
                        // We fill the buffer with numbers for the payload 
                        for( i = 4; i < BufferSize; i++ )
                        {
                            Buffer[i] = i - 4;
                        }
                        udelay( 1000 );
                        Radio_2.Send( Buffer, BufferSize );
                    }
                    else // valid reception but not a PING as expected
                    {    // Set device as master and start again
                        isMaster = true;
                        Radio_2.Rx( RX_TIMEOUT_VALUE );
                    }   
                }
            }
            State = LOWPOWER;
            break;
        case TX:
            // Indicates on a LED that we have sent a PING [Master]
            // Indicates on a LED that we have sent a PONG [Slave]
            printk("send a message 'ping' or 'pong'\r\n");
            Radio_2.Rx( RX_TIMEOUT_VALUE );
            State = LOWPOWER;
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            if( isMaster == true )
            {
                // Send the next PING frame
                Buffer[0] = 'P';
                Buffer[1] = 'I';
                Buffer[2] = 'N';
                Buffer[3] = 'G';
                for( i = 4; i < BufferSize; i++ )
                {
                    Buffer[i] = i - 4;
                }
                udelay( 1000 ); 
                Radio_2.Send( Buffer, BufferSize );
            }
            else
            {
                Radio_2.Rx( RX_TIMEOUT_VALUE );
            }
            State = LOWPOWER;
            break;
        case TX_TIMEOUT:
            Radio_2.Rx( RX_TIMEOUT_VALUE );
            State = LOWPOWER;
            break;
        case LOWPOWER:
        default:
            // Set low power
            break;
        }
    
        //TimerLowPowerHandler( );
		msleep(100);
	}
	return 0;
}