/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - demo main module to test IRMP decoder on PIC18F4520 with XC8 compiler
 *
 * Copyright (c) 2009-2016 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmp-main-pic-xc8.c,v 1.1 2016/01/12 11:55:05 fm Exp $
 *
 * This demo module is runnable on PIC18F4520 with XC8 compiler
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include "irmp.h"

#ifndef F_CPU
#error F_CPU unknown
#endif

#define _XTAL_FREQ  32000000UL                                              // 32MHz clock
#define FOSC        _XTAL_FREQ
#define FCY         FOSC / 4UL                                              // --> 8MHz

#define BAUDRATE 19200UL
#define BRG (( FCY  16  BAUDRATE ) -1UL)

#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
    IRMP_DATA irmp_data;

    irmp_init();                                                            // initialize irmp

    // infinite loop, interrupts will blink PORTD pins and handle UART communications.
    while (1)
    {
        LATBbits.LATB0 = ~LATBbits.LATB0;

        if (irmp_get_data (&irmp_data))
        {
            // ir signal decoded, do something here...
            // irmp_data.protocol is the protocol, see irmp.h
            // irmp_data.address is the address/manufacturer code of ir sender
            // irmp_data.command is the command code
            // irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
            printf("proto %d addr %d cmd %d\n", irmp_data.protocol, irmp_data.address, irmp_data.command );
        }
    }
}

void interrupt high_priority high_isr(void)
{
    if (TMR2IF)
    {
        TMR2IF = 0;                                                         // clear Timer 0 interrupt flag
        irmp_ISR();
    }
}
