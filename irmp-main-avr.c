/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-avr.c - demo main module to test IRMP decoder on AVR
 *
 * Copyright (c) 2009-2018 Frank Meyer - frank(at)fli4l.de
 *
 * This demo module is runnable on AVRs
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
#error F_CPU unknown
#endif

static void
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
    IRMP_DATA   irmp_data;

    irmp_init();                                                            // initialize IRMP
    timer1_init();                                                          // initialize timer1

    sei ();                                                                 // enable interrupts

    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            ;                                                               // got an IR message, do something
        }
    }
}
