/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - demo main module to test irmp decoder
 *
 * Copyright (c) 2009-2013 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: main.c,v 1.17 2013/01/17 07:33:14 fm Exp $
 *
 * This demo module is runnable on AVRs and LM4F120 Launchpad (ARM Cortex M4)
 *
 * ATMEGA88 @ 8 MHz internal RC      Osc with BODLEVEL 4.3V: lfuse: 0xE2 hfuse: 0xDC efuse: 0xF9
 * ATMEGA88 @ 8 MHz external Crystal Osc with BODLEVEL 4.3V: lfuse: 0xFF hfuse: 0xDC efuse: 0xF9
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include "irmp.h"

#ifndef F_CPU
#error F_CPU unkown
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * ATMEL AVR part:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (ATMEL_AVR)

void
timer1_init (void)
{
#if defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)                // ATtiny45 / ATtiny85:

#if F_CPU >= 16000000L
    OCR1C   =  (F_CPU / F_INTERRUPTS / 8) - 1;                              // compare value: 1/15000 of CPU frequency, presc = 8
    TCCR1   = (1 << CTC1) | (1 << CS12);                                    // switch CTC Mode on, set prescaler to 8
#else
    OCR1C   =  (F_CPU / F_INTERRUPTS / 4) - 1;                              // compare value: 1/15000 of CPU frequency, presc = 4
    TCCR1   = (1 << CTC1) | (1 << CS11) | (1 << CS10);                      // switch CTC Mode on, set prescaler to 4
#endif

#else                                                                       // ATmegaXX:
    OCR1A   =  (F_CPU / F_INTERRUPTS) - 1;                                  // compare value: 1/15000 of CPU frequency
    TCCR1B  = (1 << WGM12) | (1 << CS10);                                   // switch CTC Mode on, set prescaler to 1
#endif

#ifdef TIMSK1
    TIMSK1  = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
#else
    TIMSK   = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
#endif
}

#ifdef TIM1_COMPA_vect                                                      // ATtiny84
#define COMPA_VECT  TIM1_COMPA_vect
#else
#define COMPA_VECT  TIMER1_COMPA_vect                                       // ATmega
#endif

ISR(COMPA_VECT)                                                             // Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
  (void) irmp_ISR();                                                        // call irmp ISR
  // call other timer interrupt routines...
}

int
main (void)
{
    IRMP_DATA irmp_data;

    irmp_init();                                                            // initialize irmp
    timer1_init();                                                          // initialize timer1
    sei ();                                                                 // enable interrupts

    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            // ir signal decoded, do something here...
            // irmp_data.protocol is the protocol, see irmp.h
            // irmp_data.address is the address/manufacturer code of ir sender
            // irmp_data.command is the command code
            // irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
        }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * LM4F120 Launchpad (ARM Cortex M4):
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(STELLARIS_ARM_CORTEX_M4)

void
timer1_init (void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER);

    TimerLoadSet(TIMER1_BASE, TIMER_A, (F_CPU / F_INTERRUPTS) -1);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER1_BASE, TIMER_A);
    // Important: Timer1IntHandler has to be configured in startup_ccs.c !
}

void
Timer1IntHandler(void)                                                      // Timer1 Interrupt Handler
{
  (void) irmp_ISR();                                                        // call irmp ISR
  // call other timer interrupt routines...
}

int
main (void)
{
    IRMP_DATA irmp_data;

    ROM_FPUEnable();
    ROM_FPUStackingEnable();
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    irmp_init();                                                            // initialize irmp
    timer1_init();                                                          // initialize timer1
    sei ();                                                                 // enable interrupts

    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            // ir signal decoded, do something here...
            // irmp_data.protocol is the protocol, see irmp.h
            // irmp_data.address is the address/manufacturer code of ir sender
            // irmp_data.command is the command code
            // irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
        }
    }
}

#endif
