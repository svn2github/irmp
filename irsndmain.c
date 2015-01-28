/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndmain.c - demo main module to test IRSND encoder on AVRs
 *
 * Copyright (c) 2010-2015 Frank Meyer - frank(at)fli4l.de
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
#include "irsnd.h"

#ifndef F_CPU
#  error F_CPU unknown
#endif

void
timer1_init (void)
{
#if defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)                // ATtiny45 / ATtiny85:
    OCR1C   =  (F_CPU / F_INTERRUPTS / 4) - 1;                              // compare value: 1/15000 of CPU frequency, presc = 4
    TCCR1   = (1 << CTC1) | (1 << CS11) | (1 << CS10);                      // switch CTC Mode on, set prescaler to 4
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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * timer 1 compare handler, called every 1/10000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
ISR(COMPA_VECT)                                                             // Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
    (void) irsnd_ISR();                                                     // call irsnd ISR
    // call other timer interrupt routines here...
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MAIN: main routine
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
int
main (void)
{
    IRMP_DATA irmp_data;

    irsnd_init();                                                           // initialize irsnd
    timer1_init();                                                          // initialize timer
    sei ();                                                                 // enable interrupts

    for (;;)
    {
        irmp_data.protocol = IRMP_NEC_PROTOCOL;                             // use NEC protocol
        irmp_data.address  = 0x00FF;                                        // set address to 0x00FF
        irmp_data.command  = 0x0001;                                        // set command to 0x0001
        irmp_data.flags    = 0;                                             // don't repeat frame

        irsnd_send_data (&irmp_data, TRUE);                                 // send frame, wait for completion
        _delay_ms (1000);
    }
}
