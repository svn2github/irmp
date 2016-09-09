/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-avr-uart.c - demo main module to test IRMP decoder on AVR with UART
 *
 * Copyright (c) 2009-2016 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmp-main-avr-uart.c,v 1.2 2016/09/09 08:01:11 fm Exp $
 *
 * This demo module is runnable on AVRs with UART
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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * ATMEL AVR part:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define BAUD  9600L
#include <util/setbaud.h>

#ifdef UBRR0H

#define UART0_UBRRH                             UBRR0H
#define UART0_UBRRL                             UBRR0L
#define UART0_UCSRA                             UCSR0A
#define UART0_UCSRB                             UCSR0B
#define UART0_UCSRC                             UCSR0C
#define UART0_UDRE_BIT_VALUE                    (1<<UDRE0)
#define UART0_UCSZ1_BIT_VALUE                   (1<<UCSZ01)
#define UART0_UCSZ0_BIT_VALUE                   (1<<UCSZ00)
#ifdef URSEL0
#define UART0_URSEL_BIT_VALUE                   (1<<URSEL0)
#else
#define UART0_URSEL_BIT_VALUE                   (0)
#endif
#define UART0_TXEN_BIT_VALUE                    (1<<TXEN0)
#define UART0_UDR                               UDR0
#define UART0_U2X                               U2X0

#else

#define UART0_UBRRH                             UBRRH
#define UART0_UBRRL                             UBRRL
#define UART0_UCSRA                             UCSRA
#define UART0_UCSRB                             UCSRB
#define UART0_UCSRC                             UCSRC
#define UART0_UDRE_BIT_VALUE                    (1<<UDRE)
#define UART0_UCSZ1_BIT_VALUE                   (1<<UCSZ1)
#define UART0_UCSZ0_BIT_VALUE                   (1<<UCSZ0)
#ifdef URSEL
#define UART0_URSEL_BIT_VALUE                   (1<<URSEL)
#else
#define UART0_URSEL_BIT_VALUE                   (0)
#endif
#define UART0_TXEN_BIT_VALUE                    (1<<TXEN)
#define UART0_UDR                               UDR
#define UART0_U2X                               U2X

#endif //UBRR0H

static void
uart_init (void)
{
    UART0_UBRRH = UBRRH_VALUE;                                                                      // set baud rate
    UART0_UBRRL = UBRRL_VALUE;

#if USE_2X
    UART0_UCSRA |= (1<<UART0_U2X);
#else
    UART0_UCSRA &= ~(1<<UART0_U2X);
#endif

    UART0_UCSRC = UART0_UCSZ1_BIT_VALUE | UART0_UCSZ0_BIT_VALUE | UART0_URSEL_BIT_VALUE;
    UART0_UCSRB |= UART0_TXEN_BIT_VALUE;                                                            // enable UART TX
}

static void
uart_putc (unsigned char ch)
{
    while (!(UART0_UCSRA & UART0_UDRE_BIT_VALUE))
    {
        ;
    }

    UART0_UDR = ch;
}

static void
uart_puts (char * s)
{
    while (*s)
    {
        uart_putc (*s);
        s++;
    }
}

static void
uart_puts_P (PGM_P s)
{
    uint8_t ch;

    while ((ch = pgm_read_byte(s)) != '\0')
    {
        uart_putc (ch);
        s++;
    }
}

static char *
itoh (char * buf, uint8_t digits, uint16_t number)
{
    for (buf[digits] = 0; digits--; number >>= 4)
    {
        buf[digits] = "0123456789ABCDEF"[number & 0x0F];
    }
    return buf;
}

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
    char        buf[3];

    irmp_init();                                                            // initialize irmp
    timer1_init();                                                          // initialize timer1
    uart_init();                                                            // initialize uart

    sei ();                                                                 // enable interrupts

    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            uart_puts_P (PSTR("protocol: 0x"));
            itoh (buf, 2, irmp_data.protocol);
            uart_puts (buf);

#if IRMP_PROTOCOL_NAMES == 1
            uart_puts_P (PSTR("   "));
            uart_puts_P (pgm_read_word (&(irmp_protocol_names[irmp_data.protocol])));
#endif

            uart_puts_P (PSTR("   address: 0x"));
            itoh (buf, 4, irmp_data.address);
            uart_puts (buf);

            uart_puts_P (PSTR("   command: 0x"));
            itoh (buf, 4, irmp_data.command);
            uart_puts (buf);

            uart_puts_P (PSTR("   flags: 0x"));
            itoh (buf, 2, irmp_data.flags);
            uart_puts (buf);

            uart_puts_P (PSTR("\r\n"));
        }
    }
}
