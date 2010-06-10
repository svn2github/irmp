/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndmain.c - demo main module to test irmp decoder
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * ATMEGA88 @ 8 MHz
 *
 * Fuses: lfuse: 0xE2 hfuse: 0xDC efuse: 0xF9
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uncomment this for codevision compiler:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
// #define CODEVISION                                                   // to use Codevision Compiler instead of gcc

#ifdef CODEVISION
#include <mega88.h>
#include <stdio.h>
#define uint8_t     unsigned char
#define uint16_t    unsigned int
#define F_CPU       8000000            // change for Codevision here, if you use WinAVR, use Project -> Configuration Options instead

// register values from datasheet for ATMega88
#define OCIE1A      1
#define WGM12       3
#define CS10        0
#define UDRE0       5
#define TXEN0       3

#include "irmp.h"
#include "isnd.h"
#include "irmp.c"
#include "isnd.c"

#else // gcc compiler

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "irmp.h"
#include "irsndconfig.h"
#include "irsnd.h"

#endif  // CODEVISION

#ifndef F_CPU
#error F_CPU unkown
#endif

void
timer_init (void)
{
#ifdef CODEVISION
  OCR1AH  = ((F_CPU / F_INTERRUPTS) >> 8) & 0xFF;                           // compare value: 1/10000 of CPU frequency (upper byte)
  OCR1AL  = ((F_CPU / F_INTERRUPTS) - 1)  & 0xFF;                           // compare value: 1/10000 of CPU frequency (lower byte)
#else  // gcc
  OCR1A   =  (F_CPU / F_INTERRUPTS) - 1;                                    // compare value: 1/10000 of CPU frequency
#endif  // CODEVISION
  TCCR1B  = (1 << WGM12) | (1 << CS10);                                     // switch CTC Mode on, set prescaler to 1

#if defined (__AVR_ATmega8__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega64__) || defined (__AVR_ATmega162__)
  TIMSK  = 1 << OCIE1A;                                                     // OCIE1A: Interrupt by timer compare (use TIMSK for ATMEGA162)
#else
  TIMSK1  = 1 << OCIE1A;                                                    // OCIE1A: Interrupt by timer compare (use TIMSK for ATMEGA162)
#endif	// __AVR...
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * timer 1 compare handler, called every 1/10000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
// Timer 1 output compare A interrupt service routine
#ifdef CODEVISION
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
#else  // CODEVISION
ISR(TIMER1_COMPA_vect)
#endif  // CODEVISION
{
    (void) irsnd_ISR();                                                     // call irsnd ISR
  // call other timer interrupt routines...
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MAIN: main routine
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef CODEVISION
// This is the main routine if you use Codevision C Compiler
void
main (void)
{
  IRMP_DATA irmp_data;

  #pragma optsize-
  // crystal oscillator division factor: 1
  CLKPR=0x80;
  CLKPR=0x00;
  #ifdef _OPTIMIZE_SIZE_
  #pragma optsize+
  #endif

  irsnd_init();         // initialize irsnd
  timer_init();                                                             // initialize timer
  #asm("sei");                                                                // enable interrupts

  for (;;)
  {
    irmp_data.protocol = IRMP_NEC_PROTOCOL;
    irmp_data.address  = 0x00FF;
    irmp_data.command  = 0x0001;
    irmp_data.flags    = 0;

    irsnd_send_data (&irmp_data);
    _delay_ms (1000);
  }
}

#else  // gcc

// This is the main routine if you use GCC Compiler
int
main (void)
{
  IRMP_DATA irmp_data;

  irsnd_init();                                                             // initialize irsnd
  timer_init();                                                             // initialize timer
  sei ();                                                                   // enable interrupts

  for (;;)
  {
    irmp_data.protocol = IRMP_NEC_PROTOCOL;
    irmp_data.address  = 0x00FF;
    irmp_data.command  = 0x0001;
    irmp_data.flags    = 0;

    irsnd_send_data (&irmp_data, TRUE);
    _delay_ms (1000);
  }
}

#endif  // CODEVISION / gcc
