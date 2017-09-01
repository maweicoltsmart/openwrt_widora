/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: sx1276_1 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __SI4438_ARCH_H__
#define __SI4438_ARCH_H__

extern unsigned int si4438_irq;
void si4438_IoInit( void );
void si4438_IoIrqInit(void);
#endif // __sx1276_1_ARCH_H__
