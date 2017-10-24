#ifndef __TYPE_DEF__
#define __TYPE_DEF__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif

#define BUFFER_SIZE 1024

extern bool lora_rx_done;
extern uint8_t lora_rx_len;
extern uint8_t radio2tcpbuffer[];

struct msg_st
{
    long int msg_type;
    char text[BUFFER_SIZE];
};

#endif
