#ifndef __RADIO_DEFAULT_PARAM_H__
#define __RADIO_DEFAULT_PARAM_H__

#include <linux/stddef.h>
#include <linux/types.h>

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64          // Define the payload size here
#define RF_FREQUENCY                                470000000   // Hz
#define TX_OUTPUT_POWER                             20          // dBm
#define LORA_BANDWIDTH                              0           // [0: 125 kHz,
                                                                //  1: 250 kHz,
                                                                //  2: 500 kHz,
                                                                //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7           // [SF7..SF12]
#define LORA_CODINGRATE                             1           // [1: 4/5,
                                                                //  2: 4/6,
                                                                //  3: 4/7,
                                                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8           // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         24           // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#endif