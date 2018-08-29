/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-avr-mitsu-heavy.c - demo main module to test Mitsubishi heavy airconditioner
 *
 * Copyright (c) 2016-2018 Frank Meyer - frank(at)fli4l.de
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

//static
void uart_putc (unsigned char ch)
{
    while (!(UART0_UCSRA & UART0_UDRE_BIT_VALUE))
    {
        ;
    }

    UART0_UDR = ch;
}

static
void uart_puts (char * s)
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

static FILE mystdout = FDEV_SETUP_STREAM(uart_putc, 0,_FDEV_SETUP_RW);

int
main (void)
{
    IRMP_DATA   irmp_data;
    char        buf[5];

    irmp_init();                                                            // initialize irmp
    timer1_init();                                                          // initialize timer1
    uart_init();                                                            // initialize uart
    stdout = stdin = &mystdout;

    sei ();                                                                 // enable interrupts
    printf_P(PSTR("Start\n"));
    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            printf_P(PSTR("protokoll: %d "),irmp_data.protocol);
            #if IRMP_PROTOCOL_NAMES == 1
              uart_puts_P (pgm_read_word (&(irmp_protocol_names[irmp_data.protocol])));
            #endif
            printf_P(PSTR("   address: 0x%X"),irmp_data.address);
            printf_P(PSTR("   command: 0x%X"),irmp_data.command);
            printf_P(PSTR("   flags: 0x%X\n"),irmp_data.flags);
            if (irmp_data.protocol == IRMP_MITSU_HEAVY_PROTOCOL)
            {
              uint16_t i;
              for( i=0x8000; i>0x80; i=i>>1) {
                if (irmp_data.address &i) uart_putc('1');
                else uart_putc('0');
              }
              printf_P(PSTR("  %02X\n"),irmp_data.address >> 8);
              for( i=0x80; i; i=i>>1) {
                if (irmp_data.address &i) uart_putc('1');
                else uart_putc('0');
              }
              printf_P(PSTR("  %02X\n"),irmp_data.address & 0xff);
              for( i=0x80; i; i=i>>1) {
                if (irmp_data.command &i) uart_putc('1');
                else uart_putc('0');
              }
              printf_P(PSTR("  %02X\n"),irmp_data.command);

              if (irmp_data.command & 0b00010000) printf_P(PSTR("ON   "));
              else printf_P(PSTR("OFF  "));
              switch (irmp_data.address & 0b00000111) {
                case 0b000:  printf_P(PSTR("Auto  "));break;
                case 0b001:  printf_P(PSTR("High  "));break;
                case 0b110:  printf_P(PSTR("Med   "));break;
                case 0b010:  printf_P(PSTR("Low   "));break;
                case 0b011:  printf_P(PSTR("HiPwr "));break;
                case 0b111:  printf_P(PSTR("Eco   "));break;
              }
              switch (irmp_data.command & 0b11100000) {
                case 0:           printf_P(PSTR("Auto  "));break;
                case 0b10000000:  printf_P(PSTR("Cool  "));break;
                case 0b00100000:  printf_P(PSTR("Heat  "));break;
                case 0b01000000:  printf_P(PSTR("Dehum "));break;
                case 0b11000000:  printf_P(PSTR("Vent  "));break;
              }
              switch (irmp_data.command & 0b00001111) {
                case 0b1000: printf_P(PSTR("18°")); break;
                case 0b0100: printf_P(PSTR("19°")); break;
                case 0b1100: printf_P(PSTR("20°")); break;
                case 0b0010: printf_P(PSTR("21°")); break;
                case 0b1010: printf_P(PSTR("22°")); break;
                case 0b0110: printf_P(PSTR("23°")); break;
                case 0b1110: printf_P(PSTR("24°")); break;
                case 0b0001: printf_P(PSTR("25°")); break;
                case 0b1001: printf_P(PSTR("26°")); break;
                case 0b0101: printf_P(PSTR("27°")); break;
                case 0b1101: printf_P(PSTR("28°")); break;
                case 0b0011: printf_P(PSTR("29°")); break;
                case 0b1011: printf_P(PSTR("30°")); break;
              }
              uart_putc('\n');
            }
        }
    }
}
