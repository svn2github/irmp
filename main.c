/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - demo main module to test irmp decoder
 *
 * Copyright (c) 2009-2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: main.c,v 1.7 2010/06/22 08:33:21 fm Exp $
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
#include "irmp.c"

#else // gcc compiler

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "irmp.h"
#include "irmpconfig.h"


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
  TIMSK  = 1 << OCIE1A;                                                     // OCIE1A: Interrupt by timer compare
#else
  TIMSK1  = 1 << OCIE1A;                                                    // OCIE1A: Interrupt by timer compare
#endif  // __AVR...
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
  (void) irmp_ISR();                                                        // call irmp ISR
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
  static uint8_t *Proto[]={"SIRCS","NEC","SAMSUNG","MATSUSH","KASEIKYO","RECS80","RC5(x)","DENON","RC6","SAMSG32","APPLE"};
  #define IRMP_APPLE_ADDRESS 0x77E1 


  #if IRMP_LOGGING == 0
  // USART initialization has to be done here if Logging is off
  // Communication Parameters: 8 Data, 1 Stop, No Parity
  // USART Receiver: Off
  // USART Transmitter: On
  // USART0 Mode: Asynchronous
  // USART Baud Rate: 9600
  #define BAUDRATE 9600L
  UCSR0A=0x00;
  UCSR0B=0x08;
  UCSR0C=0x06;
  UBRR0H = ((F_CPU+BAUDRATE*8)/(BAUDRATE*16)-1) >> 8;            // store baudrate (upper byte)
  UBRR0L = ((F_CPU+BAUDRATE*8)/(BAUDRATE*16)-1) & 0xFF;    
  #endif

  irmp_init();         // initialize rc5

  printf("IRMP V1.0\n");
  #if IRMP_LOGGING == 1
  printf("Logging Mode\n");
  #endif

  timer_init();                                                             // initialize timer
  #asm("sei");                                                                // enable interrupts

  for (;;)
  {
    if (irmp_get_data (&irmp_data))
    {
        // ir signal decoded, do something here...
        // irmp_data.protocol is the protocol, see irmp.h
        // irmp_data.address is the address/manufacturer code of ir sender
        // irmp_data.command is the command code
        #if IRMP_LOGGING != 1
        if((irmp_data.protocol == IRMP_NEC_PROTOCOL) && (irmp_data.address == IRMP_APPLE_ADDRESS))
          printf("Code: Apple\n");
        else printf("Code: %s\n",Proto[irmp_data.protocol-1]);
        printf("Address: 0x%.2X\n",irmp_data.address);
        printf("Command: 0x%.2X\n\n",irmp_data.command);
        #endif
    }
  }
}

#else  // gcc

// This is the main routine if you use GCC Compiler
int
main (void)
{
  IRMP_DATA irmp_data;

  irmp_init();                                                              // initialize rc5
  timer_init();                                                             // initialize timer
  sei ();                                                                   // enable interrupts

  for (;;)
  {
    if (irmp_get_data (&irmp_data))
    {
        // ir signal decoded, do something here...
        // irmp_data.protocol is the protocol, see irmp.h
        // irmp_data.address is the address/manufacturer code of ir sender
        // irmp_data.command is the command code
    }
  }
}

#endif  // CODEVISION / gcc
