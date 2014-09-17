/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - demo main module to test irmp decoder
 *
 * Copyright (c) 2009-2014 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: main.c,v 1.21 2014/09/17 09:44:47 fm Exp $
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

#include "irmp.h"
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

static uint8_t
itox (uint8_t val)
{
    uint8_t rtc;

    val &= 0x0F;

    if (val <= 9)
    {
        rtc = val + '0';
    }
    else
    {
        rtc = val - 10 + 'A';
    }
    return (rtc);
}

static void
itoxx (char * xx, unsigned char i)
{
    *xx++ = itox (i >> 4);
    *xx++ = itox (i & 0x0F);
    *xx = '\0';
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
            itoxx (buf, irmp_data.protocol);
            uart_puts (buf);

#if IRMP_PROTOCOL_NAMES == 1
            uart_puts_P (PSTR("   "));
            uart_puts_P (pgm_read_word (&(irmp_protocol_names[irmp_data.protocol])));
#endif

            uart_puts_P (PSTR("   address: 0x"));
            itoxx (buf, irmp_data.address >> 8);
            uart_puts (buf);
            itoxx (buf, irmp_data.address & 0xFF);
            uart_puts (buf);

            uart_puts_P (PSTR("   command: 0x"));
            itoxx (buf, irmp_data.command >> 8);
            uart_puts (buf);
            itoxx (buf, irmp_data.command & 0xFF);
            uart_puts (buf);

            uart_puts_P (PSTR("   flags: 0x"));
            itoxx (buf, irmp_data.flags);
            uart_puts (buf);

            uart_puts_P (PSTR("\r\n"));
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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PIC18F4520 with XC8 compiler:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (__XC8)

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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * STM32:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(ARM_STM32)

uint32_t
SysCtlClockGet(void)
{
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    return RCC_ClocksStatus.SYSCLK_Frequency;
}

void
timer2_init (void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 7;
    TIM_TimeBaseStructure.TIM_Prescaler = ((F_CPU / F_INTERRUPTS)/8) - 1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void
TIM2_IRQHandler(void)                                                  // Timer2 Interrupt Handler
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  (void) irmp_ISR();                                                        // call irmp ISR
  // call other timer interrupt routines...
}

int
main (void)
{
    IRMP_DATA irmp_data;
        
    irmp_init();                                                            // initialize irmp
    timer2_init();                                                          // initialize timer2

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
