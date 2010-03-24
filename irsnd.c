/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * @file irsnd.c
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifdef unix                                                                 // test/debug on linux/unix
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define DEBUG
#define F_CPU 8000000L

#else // not unix:

#ifdef WIN32                                                                 // test/debug on windows
#include <stdio.h>
#define F_CPU 8000000L
typedef unsigned char    uint8_t;
typedef unsigned short    uint16_t;
#define DEBUG

#else

#ifdef CODEVISION
  #define COM2A0 6
  #define WGM21  1
  #define CS20   0
#else
  #include <inttypes.h>
  #include <avr/io.h>
  #include <util/delay.h>
  #include <avr/pgmspace.h>
#endif // CODEVISION

#endif // WIN32
#endif // unix

#include "irmp.h"
#include "irsnd.h"

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more encoders.
 * This saves program space.
 * 1 enable  decoder
 * 0 disable decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRSND_SUPPORT_SIRCS_PROTOCOL            1       // flag: support SIRCS                  uses ~150 bytes
#define IRSND_SUPPORT_NEC_PROTOCOL              1       // flag: support NEC                    uses ~100 bytes
#define IRSND_SUPPORT_SAMSUNG_PROTOCOL          1       // flag: support Samsung + Samsung32    uses ~300 bytes
#define IRSND_SUPPORT_MATSUSHITA_PROTOCOL       1       // flag: support Matsushita             uses ~150 bytes
#define IRSND_SUPPORT_KASEIKYO_PROTOCOL         0       // flag: support Kaseikyo               NOT SUPPORTED YET!
#define IRSND_SUPPORT_RECS80_PROTOCOL           1       // flag: support RECS80                 uses ~100 bytes
#define IRSND_SUPPORT_RC5_PROTOCOL              1       // flag: support RC5                    uses ~250 bytes
#define IRSND_SUPPORT_DENON_PROTOCOL            1       // flag: support DENON                  uses ~200 bytes
#define IRSND_SUPPORT_RC6_PROTOCOL              0       // flag: support RC6                    NOT SUPPORTED YET!
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        1       // flag: support RECS80EXT              uses ~100 bytes
#define IRSND_SUPPORT_NUBERT_PROTOCOL           1       // flag: support NUBERT                 uses ~100 bytes


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega644P__)
#define IRSND_PORT          PORTD                       // port D
#define IRSND_DDR           DDRD                        // ddr D
#define IRSND_BIT           7                           // OC2A
#else
#define IRSND_PORT          PORTB                       // port B
#define IRSND_DDR           DDRB                        // ddr B
#define IRSND_BIT           3                           // OC2A
#endif // __AVR...


#define SIRCS_START_BIT_PULSE_LEN               (uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PULSE_TIME + 0.5)
#define SIRCS_START_BIT_PAUSE_LEN               (uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PAUSE_TIME + 0.5)
#define SIRCS_1_PULSE_LEN                       (uint8_t)(F_INTERRUPTS * SIRCS_1_PULSE_TIME + 0.5)
#define SIRCS_0_PULSE_LEN                       (uint8_t)(F_INTERRUPTS * SIRCS_0_PULSE_TIME + 0.5)
#define SIRCS_PAUSE_LEN                         (uint8_t)(F_INTERRUPTS * SIRCS_PAUSE_TIME + 0.5)
#define SIRCS_REPETITION_LEN                    (uint16_t)(F_INTERRUPTS * SIRCS_REPETITION_TIME + 0.5)       // use uint16_t!

#define NEC_START_BIT_PULSE_LEN                 (uint8_t)(F_INTERRUPTS * NEC_START_BIT_PULSE_TIME + 0.5)
#define NEC_START_BIT_PAUSE_LEN                 (uint8_t)(F_INTERRUPTS * NEC_START_BIT_PAUSE_TIME + 0.5)
#define NEC_PULSE_LEN                           (uint8_t)(F_INTERRUPTS * NEC_PULSE_TIME + 0.5)
#define NEC_1_PAUSE_LEN                         (uint8_t)(F_INTERRUPTS * NEC_1_PAUSE_TIME + 0.5)
#define NEC_0_PAUSE_LEN                         (uint8_t)(F_INTERRUPTS * NEC_0_PAUSE_TIME + 0.5)

#define SAMSUNG_START_BIT_PULSE_LEN             (uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PULSE_TIME + 0.5)
#define SAMSUNG_START_BIT_PAUSE_LEN             (uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PAUSE_TIME + 0.5)
#define SAMSUNG_PULSE_LEN                       (uint8_t)(F_INTERRUPTS * SAMSUNG_PULSE_TIME + 0.5)
#define SAMSUNG_1_PAUSE_LEN                     (uint8_t)(F_INTERRUPTS * SAMSUNG_1_PAUSE_TIME + 0.5)
#define SAMSUNG_0_PAUSE_LEN                     (uint8_t)(F_INTERRUPTS * SAMSUNG_0_PAUSE_TIME + 0.5)

#define MATSUSHITA_START_BIT_PULSE_LEN          (uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PULSE_TIME + 0.5)
#define MATSUSHITA_START_BIT_PAUSE_LEN          (uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PAUSE_TIME + 0.5)
#define MATSUSHITA_PULSE_LEN                    (uint8_t)(F_INTERRUPTS * MATSUSHITA_PULSE_TIME + 0.5)
#define MATSUSHITA_1_PAUSE_LEN                  (uint8_t)(F_INTERRUPTS * MATSUSHITA_1_PAUSE_TIME + 0.5)
#define MATSUSHITA_0_PAUSE_LEN                  (uint8_t)(F_INTERRUPTS * MATSUSHITA_0_PAUSE_TIME + 0.5)

#define RECS80_START_BIT_PULSE_LEN              (uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PULSE_TIME + 0.5)
#define RECS80_START_BIT_PAUSE_LEN              (uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PAUSE_TIME + 0.5)
#define RECS80_PULSE_LEN                        (uint8_t)(F_INTERRUPTS * RECS80_PULSE_TIME + 0.5)
#define RECS80_1_PAUSE_LEN                      (uint8_t)(F_INTERRUPTS * RECS80_1_PAUSE_TIME + 0.5)
#define RECS80_0_PAUSE_LEN                      (uint8_t)(F_INTERRUPTS * RECS80_0_PAUSE_TIME + 0.5)

#define RC5_START_BIT_LEN                       (uint8_t)(F_INTERRUPTS * RC5_BIT_TIME + 0.5)
#define RC5_BIT_LEN                             (uint8_t)(F_INTERRUPTS * RC5_BIT_TIME + 0.5)

#define RC6_START_BIT_PULSE_LEN                 (uint8_t)(F_INTERRUPTS * RC6_START_BIT_PULSE_TIME + 0.5)
#define RC6_START_BIT_PAUSE_LEN                 (uint8_t)(F_INTERRUPTS * RC6_START_BIT_PAUSE_TIME + 0.5)
#define RC6_TOGGLE_BIT_LEN                      (uint8_t)(F_INTERRUPTS * RC6_TOGGLE_BIT_TIME + 0.5)
#define RC6_BIT_LEN                             (uint8_t)(F_INTERRUPTS * RC6_BIT_TIME + 0.5)

#define DENON_PULSE_LEN                         (uint8_t)(F_INTERRUPTS * DENON_PULSE_TIME + 0.5)
#define DENON_1_PAUSE_LEN                       (uint8_t)(F_INTERRUPTS * DENON_1_PAUSE_TIME + 0.5)
#define DENON_0_PAUSE_LEN                       (uint8_t)(F_INTERRUPTS * DENON_0_PAUSE_TIME + 0.5)
#define DENON_REPETITION_LEN                    (uint16_t)(F_INTERRUPTS * DENON_REPETITION_TIME + 0.5)       // use uint16_t!

#define RECS80EXT_START_BIT_PULSE_LEN           (uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PULSE_TIME + 0.5)
#define RECS80EXT_START_BIT_PAUSE_LEN           (uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PAUSE_TIME + 0.5)
#define RECS80EXT_PULSE_LEN                     (uint8_t)(F_INTERRUPTS * RECS80EXT_PULSE_TIME + 0.5)
#define RECS80EXT_1_PAUSE_LEN                   (uint8_t)(F_INTERRUPTS * RECS80EXT_1_PAUSE_TIME + 0.5)
#define RECS80EXT_0_PAUSE_LEN                   (uint8_t)(F_INTERRUPTS * RECS80EXT_0_PAUSE_TIME + 0.5)

#define NUBERT_START_BIT_PULSE_LEN              (uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PULSE_TIME + 0.5)
#define NUBERT_START_BIT_PAUSE_LEN              (uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PAUSE_TIME + 0.5)
#define NUBERT_1_PULSE_LEN                      (uint8_t)(F_INTERRUPTS * NUBERT_1_PULSE_TIME + 0.5)
#define NUBERT_1_PAUSE_LEN                      (uint8_t)(F_INTERRUPTS * NUBERT_1_PAUSE_TIME + 0.5)
#define NUBERT_0_PULSE_LEN                      (uint8_t)(F_INTERRUPTS * NUBERT_0_PULSE_TIME + 0.5)
#define NUBERT_0_PAUSE_LEN                      (uint8_t)(F_INTERRUPTS * NUBERT_0_PAUSE_TIME + 0.5)
#define NUBERT_REPETITION_LEN                   (uint16_t)(F_INTERRUPTS * NUBERT_REPETITION_TIME + 0.5)       // use uint16_t!

#define IRSND_FREQ_32_KHZ                       (uint8_t) ((F_CPU / 32000 / 2) - 1)
#define IRSND_FREQ_36_KHZ                       (uint8_t) ((F_CPU / 36000 / 2) - 1)
#define IRSND_FREQ_38_KHZ                       (uint8_t) ((F_CPU / 38000 / 2) - 1)
#define IRSND_FREQ_40_KHZ                       (uint8_t) ((F_CPU / 40000 / 2) - 1)
#define IRSND_FREQ_56_KHZ                       (uint8_t) ((F_CPU / 56000 / 2) - 1)

static volatile uint8_t                         irsnd_busy;
static volatile uint8_t                         irsnd_protocol;
static volatile uint8_t                         irsnd_buffer[5];
static volatile uint8_t                         irsnd_is_on = FALSE;

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Switch PWM on
 *  @details  Switches PWM on with a narrow spike on all 3 channels -> leds glowing
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
irsnd_on (void)
{
    if (! irsnd_is_on)
    {
#ifndef DEBUG
#if defined (__AVR_ATmega32__)
        TCCR2 |= (1<<COM20)|(1<<WGM21);                 // = 0x42: toggle OC2A on compare match, clear Timer 2 at compare match OCR2A
#else
        TCCR2A |= (1<<COM2A0)|(1<<WGM21);               // = 0x42: toggle OC2A on compare match, clear Timer 2 at compare match OCR2A
#endif	// __AVR...
#endif // DEBUG
        irsnd_is_on = TRUE;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Switch PWM off
 *  @details  Switches PWM off
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
irsnd_off (void)
{
    if (irsnd_is_on)
    {
#ifndef DEBUG
#if defined (__AVR_ATmega32__)
        TCCR2 &= ~(1<<COM20);                                                           // normal port operation, OC2A disconnected.
#else
        TCCR2A &= ~(1<<COM2A0);                                                         // normal port operation, OC2A disconnected.
#endif	// __AVR...
        IRSND_PORT  &= ~(1<<IRSND_BIT);                                                 // set IRSND_BIT to low
#endif // DEBUG
        irsnd_is_on = FALSE;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Set PWM frequency
 *  @details  sets pwm frequency
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
irsnd_set_freq (uint8_t freq)
{
#ifndef DEBUG
#if defined (__AVR_ATmega32__)
    OCR2 = freq;
#else
    OCR2A = freq;
#endif	// __AVR...
#endif // DEBUG
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initialize the PWM
 *  @details  Configures 0CR0A, 0CR0B and 0CR2B as PWM channels
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
irsnd_init (void)
{
#ifndef DEBUG
    IRSND_PORT &= ~(1<<IRSND_BIT);                                                  // set IRSND_BIT to low
    IRSND_DDR |= (1<<IRSND_BIT);                                                    // set IRSND_BIT to output

#if defined (__AVR_ATmega32__)
    TCCR2 = (1<<WGM21);                                                             // CTC mode
    TCCR2 |= (1<<CS20);                                                             // 0x01, start Timer 2, no prescaling
#else
    TCCR2A = (1<<WGM21);                                                            // CTC mode
    TCCR2B |= (1<<CS20);                                                            // 0x01, start Timer 2, no prescaling
#endif	// __AVR...    

    irsnd_set_freq (IRSND_FREQ_36_KHZ);                                             // default frequency
#endif // DEBUG
}

uint8_t
irsnd_is_busy (void)
{
    return irsnd_busy;
}

static uint16_t
bitsrevervse (uint16_t x, uint8_t len)
{
    uint16_t    xx = 0;

    while(len)
    {
        xx <<= 1;
        if (x & 1)
        {
            xx |= 1;
        }
        x >>= 1;
        len--;
    }
    return xx;
}


uint8_t
irsnd_send_data (IRMP_DATA * irmp_data_p)
{
#if IRSND_SUPPORT_RECS80_PROTOCOL == 1
    static uint8_t  toggle_bit_recs80;
#endif
#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1
    static uint8_t  toggle_bit_recs80ext;
#endif
#if IRSND_SUPPORT_RC5_PROTOCOL == 1
    static uint8_t  toggle_bit_rc5;
#endif
    uint16_t        address;
    uint16_t        command;

    while (irsnd_busy)
    {
        ;
    }

    irsnd_protocol  = irmp_data_p->protocol;

    switch (irsnd_protocol)
    {
#if IRSND_SUPPORT_SIRCS_PROTOCOL == 1
        case IRMP_SIRCS_PROTOCOL:
        {
            command = bitsrevervse (irmp_data_p->command, SIRCS_MINIMUM_DATA_LEN);

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = (command & 0x0FF0) >> 4;                                                         // CCCCCCCC
            irsnd_buffer[1] = (command & 0x000F) << 4;                                                         // CCCC0000
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_NEC_PROTOCOL == 1
        case IRMP_NEC_PROTOCOL:
        {
            address = bitsrevervse (irmp_data_p->address, NEC_ADDRESS_LEN);
            command = bitsrevervse (irmp_data_p->command, NEC_COMMAND_LEN);

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = (address & 0xFF00) >> 8;                                                          // AAAAAAAA
            irsnd_buffer[1] = (address & 0x00FF);                                                               // AAAAAAAA
            irsnd_buffer[2] = (command & 0xFF00) >> 8;                                                          // CCCCCCCC
            irsnd_buffer[3] = ~((command & 0xFF00) >> 8);                                                       // cccccccc
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_SAMSUNG_PROTOCOL == 1
        case IRMP_SAMSUNG_PROTOCOL:
        {
            address = bitsrevervse (irmp_data_p->address, SAMSUNG_ADDRESS_LEN);
            command = bitsrevervse (irmp_data_p->command, SAMSUNG_COMMAND_LEN);

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] =  (address & 0xFF00) >> 8;                                                         // AAAAAAAA
            irsnd_buffer[1] =  (address & 0x00FF);                                                              // AAAAAAAA
            irsnd_buffer[2] =  (command & 0x00F0) | ((command & 0xF000) >> 12);                                 // IIIICCCC
            irsnd_buffer[3] = ((command & 0x0F00) >> 4) | ((~(command & 0xF000) >> 12) & 0x0F);                 // CCCCcccc
            irsnd_buffer[4] = (~(command & 0x0F00) >> 4) & 0xF0;                                                // cccc0000
            irsnd_busy      = TRUE;
            break;
        }
        case IRMP_SAMSUNG32_PROTOCOL:
        {
            address = bitsrevervse (irmp_data_p->address, SAMSUNG_ADDRESS_LEN);
            command = bitsrevervse (irmp_data_p->command, SAMSUNG32_COMMAND_LEN);

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = (address & 0xFF00) >> 8;                                                          // AAAAAAAA
            irsnd_buffer[1] = (address & 0x00FF);                                                               // AAAAAAAA
            irsnd_buffer[2] = (command & 0xFF00) >> 8;                                                          // CCCCCCCC
            irsnd_buffer[3] = (command & 0x00FF);                                                               // CCCCCCCC
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_MATSUSHITA_PROTOCOL == 1
        case IRMP_MATSUSHITA_PROTOCOL:
        {
            address = bitsrevervse (irmp_data_p->address, MATSUSHITA_ADDRESS_LEN);
            command = bitsrevervse (irmp_data_p->command, MATSUSHITA_COMMAND_LEN);

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = (command & 0x0FF0) >> 4;                                                          // CCCCCCCC
            irsnd_buffer[1] = ((command & 0x000F) << 4) | ((address & 0x0F00) >> 8);                            // CCCCAAAA
            irsnd_buffer[2] = (address & 0x00FF);                                                               // AAAAAAAA
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_RECS80_PROTOCOL == 1
        case IRMP_RECS80_PROTOCOL:
        {
            toggle_bit_recs80 = toggle_bit_recs80 ? 0x00 : 0x40;

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = 0x80 | toggle_bit_recs80 | ((irmp_data_p->address & 0x0007) << 3) |
                              ((irmp_data_p->command & 0x0038) >> 3);                                           // STAAACCC
            irsnd_buffer[1] = (irmp_data_p->command & 0x07) << 5;                                               // CCC00000
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1
        case IRMP_RECS80EXT_PROTOCOL:
        {
            toggle_bit_recs80ext = toggle_bit_recs80ext ? 0x00 : 0x40;

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = 0x80 | toggle_bit_recs80ext | ((irmp_data_p->address & 0x000F) << 2) |
                                ((irmp_data_p->command & 0x0030) >> 4);                                         // STAAAACC
            irsnd_buffer[1] = (irmp_data_p->command & 0x0F) << 4;                                               // CCCC0000
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_RC5_PROTOCOL == 1
        case IRMP_RC5_PROTOCOL:
        {
            toggle_bit_rc5 = toggle_bit_rc5 ? 0x00 : 0x40;

            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = ((irmp_data_p->command & 0x40) ? 0x00 : 0x80) | toggle_bit_rc5 |
                                ((irmp_data_p->address & 0x001F) << 1) | ((irmp_data_p->command & 0x20) >> 5);  // CTAAAAAC
            irsnd_buffer[1] = (irmp_data_p->command & 0x1F) << 3;                                               // CCCCC000
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_DENON_PROTOCOL == 1
        case IRMP_DENON_PROTOCOL:
        {
            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = ((irmp_data_p->address & 0x1F) << 3) | ((irmp_data_p->command & 0x0380) >> 7);    // AAAAACCC
            irsnd_buffer[1] = (irmp_data_p->command & 0x7F) << 1;                                               // CCCCCCC0
            irsnd_buffer[2] = ((irmp_data_p->address & 0x1F) << 3) | (((~irmp_data_p->command) & 0x0380) >> 7); // AAAAACCC
            irsnd_buffer[3] = (~(irmp_data_p->command) & 0x7F) << 1;                                            // CCCCCCC0
            irsnd_busy      = TRUE;
            break;
        }
#endif
#if IRSND_SUPPORT_NUBERT_PROTOCOL == 1
        case IRMP_NUBERT_PROTOCOL:
        {
            irsnd_protocol  = irmp_data_p->protocol;
            irsnd_buffer[0] = irmp_data_p->command >> 2;                                                        // CCCCCCCC
            irsnd_buffer[1] = (irmp_data_p->command & 0x0003) << 6;                                             // CC000000
            irsnd_busy      = TRUE;
            break;
        }
#endif
        default:
        {
            break;
        }
    }

    return irsnd_busy;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  ISR routine
 *  @details  ISR routine, called 10000 times per second
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
irsnd_ISR (void)
{
    static uint8_t  current_bit = 0xFF;
    static uint8_t  pulse_counter;
    static uint8_t  pause_counter;
    static uint8_t  startbit_pulse_len;
    static uint8_t  startbit_pause_len;
    static uint8_t  pulse_1_len;
    static uint8_t  pause_1_len;
    static uint8_t  pulse_0_len;
    static uint8_t  pause_0_len;
    static uint8_t  has_stop_bit;
    static uint8_t  new_frame = TRUE;
    static uint8_t  complete_data_len;
    static uint8_t  n_repetitions;                                                  // number of repetitions
    static uint8_t  repetition_counter;                                             // repetition counter
    static uint16_t repetition_pause;                                               // pause before repetition, uint16_t!
    static uint16_t repetition_pause_counter;                                       // pause before repetition, uint16_t!
    uint8_t         pulse_len = 0xFF;
    uint8_t         pause_len = 0xFF;

    if (irsnd_busy)
    {
        if (current_bit == 0xFF && new_frame)                                       // start of transmission...
        {
            if (repetition_counter > 0)
            {
                repetition_pause_counter++;

                if (repetition_pause_counter >= repetition_pause)
                {
                    repetition_pause_counter = 0;

                    if (irsnd_protocol == IRMP_DENON_PROTOCOL)
                    {
                        current_bit = 16;
                        complete_data_len   = 2 * DENON_COMPLETE_DATA_LEN + 1;
                    }
                }
                else
                {
#ifdef DEBUG
                    if (irsnd_is_on)
                    {
                        putchar ('0');
                    }
                    else
                    {
                        putchar ('1');
                    }
#endif
                    return irsnd_busy;
                }
            }
            else
            {
                switch (irsnd_protocol)
                {
#if IRSND_SUPPORT_SIRCS_PROTOCOL == 1
                    case IRMP_SIRCS_PROTOCOL:
                    {
                        startbit_pulse_len  = SIRCS_START_BIT_PULSE_LEN;
                        startbit_pause_len  = SIRCS_START_BIT_PAUSE_LEN;
                        pulse_1_len         = SIRCS_1_PULSE_LEN;
                        pause_1_len         = SIRCS_PAUSE_LEN;
                        pulse_0_len         = SIRCS_0_PULSE_LEN;
                        pause_0_len         = SIRCS_PAUSE_LEN;
                        has_stop_bit        = SIRCS_STOP_BIT;
                        complete_data_len   = SIRCS_MINIMUM_DATA_LEN;
                        n_repetitions       = SIRCS_REPETITION_CNT;
                        repetition_pause    = SIRCS_REPETITION_LEN;                     // 45 ms pause
                        irsnd_set_freq (IRSND_FREQ_40_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_NEC_PROTOCOL == 1
                    case IRMP_NEC_PROTOCOL:
                    {
                        startbit_pulse_len  = NEC_START_BIT_PULSE_LEN;
                        startbit_pause_len  = NEC_START_BIT_PAUSE_LEN;
                        pulse_1_len         = NEC_PULSE_LEN;
                        pause_1_len         = NEC_1_PAUSE_LEN;
                        pulse_0_len         = NEC_PULSE_LEN;
                        pause_0_len         = NEC_0_PAUSE_LEN;
                        has_stop_bit        = NEC_STOP_BIT;
                        complete_data_len   = NEC_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_38_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_SAMSUNG_PROTOCOL == 1
                    case IRMP_SAMSUNG_PROTOCOL:
                    {
                        startbit_pulse_len  = SAMSUNG_START_BIT_PULSE_LEN;
                        startbit_pause_len  = SAMSUNG_START_BIT_PAUSE_LEN;
                        pulse_1_len         = SAMSUNG_PULSE_LEN;
                        pause_1_len         = SAMSUNG_1_PAUSE_LEN;
                        pulse_0_len         = SAMSUNG_PULSE_LEN;
                        pause_0_len         = SAMSUNG_0_PAUSE_LEN;
                        has_stop_bit        = SAMSUNG_STOP_BIT;
                        complete_data_len   = SAMSUNG_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_38_KHZ);
                        break;
                    }

                    case IRMP_SAMSUNG32_PROTOCOL:
                    {
                        startbit_pulse_len  = SAMSUNG_START_BIT_PULSE_LEN;
                        startbit_pause_len  = SAMSUNG_START_BIT_PAUSE_LEN;
                        pulse_1_len         = SAMSUNG_PULSE_LEN;
                        pause_1_len         = SAMSUNG_1_PAUSE_LEN;
                        pulse_0_len         = SAMSUNG_PULSE_LEN;
                        pause_0_len         = SAMSUNG_0_PAUSE_LEN;
                        has_stop_bit        = SAMSUNG_STOP_BIT;
                        complete_data_len   = SAMSUNG32_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_38_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_MATSUSHITA_PROTOCOL == 1
                    case IRMP_MATSUSHITA_PROTOCOL:
                    {
                        startbit_pulse_len  = MATSUSHITA_START_BIT_PULSE_LEN;
                        startbit_pause_len  = MATSUSHITA_START_BIT_PAUSE_LEN;
                        pulse_1_len         = MATSUSHITA_PULSE_LEN;
                        pause_1_len         = MATSUSHITA_1_PAUSE_LEN;
                        pulse_0_len         = MATSUSHITA_PULSE_LEN;
                        pause_0_len         = MATSUSHITA_0_PAUSE_LEN;
                        has_stop_bit        = MATSUSHITA_STOP_BIT;
                        complete_data_len   = MATSUSHITA_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_36_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_RECS80_PROTOCOL == 1
                    case IRMP_RECS80_PROTOCOL:
                    {
                        startbit_pulse_len  = RECS80_START_BIT_PULSE_LEN;
                        startbit_pause_len  = RECS80_START_BIT_PAUSE_LEN;
                        pulse_1_len         = RECS80_PULSE_LEN;
                        pause_1_len         = RECS80_1_PAUSE_LEN;
                        pulse_0_len         = RECS80_PULSE_LEN;
                        pause_0_len         = RECS80_0_PAUSE_LEN;
                        has_stop_bit        = RECS80_STOP_BIT;
                        complete_data_len   = RECS80_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_38_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1
                    case IRMP_RECS80EXT_PROTOCOL:
                    {
                        startbit_pulse_len  = RECS80EXT_START_BIT_PULSE_LEN;
                        startbit_pause_len  = RECS80EXT_START_BIT_PAUSE_LEN;
                        pulse_1_len         = RECS80EXT_PULSE_LEN;
                        pause_1_len         = RECS80EXT_1_PAUSE_LEN;
                        pulse_0_len         = RECS80EXT_PULSE_LEN;
                        pause_0_len         = RECS80EXT_0_PAUSE_LEN;
                        has_stop_bit        = RECS80EXT_STOP_BIT;
                        complete_data_len   = RECS80EXT_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_38_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_RC5_PROTOCOL == 1
                    case IRMP_RC5_PROTOCOL:
                    {
                        startbit_pulse_len  = RC5_BIT_LEN;
                        startbit_pause_len  = RC5_BIT_LEN;
                        pulse_1_len         = RC5_BIT_LEN;
                        pause_1_len         = RC5_BIT_LEN;
                        pulse_0_len         = RC5_BIT_LEN;
                        pause_0_len         = RC5_BIT_LEN;
                        has_stop_bit        = RC5_STOP_BIT;
                        complete_data_len   = RC5_COMPLETE_DATA_LEN;
                        n_repetitions       = 1;
                        irsnd_set_freq (IRSND_FREQ_36_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_DENON_PROTOCOL == 1
                    case IRMP_DENON_PROTOCOL:
                    {
                        startbit_pulse_len  = 0x00;
                        startbit_pause_len  = 0x00;
                        pulse_1_len         = DENON_PULSE_LEN;
                        pause_1_len         = DENON_1_PAUSE_LEN;
                        pulse_0_len         = DENON_PULSE_LEN;
                        pause_0_len         = DENON_0_PAUSE_LEN;
                        has_stop_bit        = DENON_STOP_BIT;
                        complete_data_len   = DENON_COMPLETE_DATA_LEN;
                        n_repetitions       = 2;
                        repetition_pause    = DENON_REPETITION_LEN;                     // 65 ms pause after 1st frame (15 bits)
                        irsnd_set_freq (IRSND_FREQ_32_KHZ);
                        break;
                    }
#endif
#if IRSND_SUPPORT_NUBERT_PROTOCOL == 1
                    case IRMP_NUBERT_PROTOCOL:
                    {
                        startbit_pulse_len  = NUBERT_START_BIT_PULSE_LEN;
                        startbit_pause_len  = NUBERT_START_BIT_PAUSE_LEN;
                        pulse_1_len         = NUBERT_1_PULSE_LEN;
                        pause_1_len         = NUBERT_1_PAUSE_LEN;
                        pulse_0_len         = NUBERT_0_PULSE_LEN;
                        pause_0_len         = NUBERT_0_PAUSE_LEN;
                        has_stop_bit        = NUBERT_STOP_BIT;
                        complete_data_len   = NUBERT_COMPLETE_DATA_LEN;
                        n_repetitions       = 2;
                        repetition_pause    = NUBERT_REPETITION_LEN;                     // 35 ms pause
                        irsnd_set_freq (IRSND_FREQ_36_KHZ);
                        break;
                    }
#endif
                    default:
                    {
                        irsnd_busy = FALSE;
                        break;
                    }
                }
            }
        }

        if (irsnd_busy)
        {
            new_frame = FALSE;
            switch (irsnd_protocol)
            {
#if IRSND_SUPPORT_SIRCS_PROTOCOL == 1
                case IRMP_SIRCS_PROTOCOL:
#endif
#if IRSND_SUPPORT_NEC_PROTOCOL == 1
                case IRMP_NEC_PROTOCOL:
#endif
#if IRSND_SUPPORT_SAMSUNG_PROTOCOL == 1
                case IRMP_SAMSUNG_PROTOCOL:
                case IRMP_SAMSUNG32_PROTOCOL:
#endif
#if IRSND_SUPPORT_MATSUSHITA_PROTOCOL == 1
                case IRMP_MATSUSHITA_PROTOCOL:
#endif
#if IRSND_SUPPORT_RECS80_PROTOCOL == 1
                case IRMP_RECS80_PROTOCOL:
#endif
#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1
                case IRMP_RECS80EXT_PROTOCOL:
#endif
#if IRSND_SUPPORT_DENON_PROTOCOL == 1
                case IRMP_DENON_PROTOCOL:
#endif
#if IRSND_SUPPORT_NUBERT_PROTOCOL == 1
                case IRMP_NUBERT_PROTOCOL:
#endif
                {
                    if (current_bit == 0xFF)                                                    // send start bit
                    {
                        pulse_len = startbit_pulse_len;
                        pause_len = startbit_pause_len;
                    }
                    else if (current_bit < complete_data_len)                                   // send n'th bit
                    {
                        if (irsnd_protocol == IRMP_SAMSUNG_PROTOCOL)
                        {
                            if (current_bit < SAMSUNG_ADDRESS_LEN)                              // send address bits
                            {
                                pulse_len = SAMSUNG_PULSE_LEN;
                                pause_len = (irsnd_buffer[current_bit / 8] & (1<<(7-(current_bit % 8)))) ?
                                                SAMSUNG_1_PAUSE_LEN : SAMSUNG_0_PAUSE_LEN;
                            }
                            else if (current_bit == SAMSUNG_ADDRESS_LEN)                        // send SYNC bit (16th bit)
                            {
                                pulse_len = SAMSUNG_PULSE_LEN;
                                pause_len = SAMSUNG_START_BIT_PAUSE_LEN;
                            }
                            else if (current_bit < SAMSUNG_COMPLETE_DATA_LEN)                   // send n'th bit
                            {
                                uint8_t cur_bit = current_bit - 1;

                                pulse_len = SAMSUNG_PULSE_LEN;
                                pause_len = (irsnd_buffer[cur_bit / 8] & (1<<(7-(cur_bit % 8)))) ?
                                                SAMSUNG_1_PAUSE_LEN : SAMSUNG_0_PAUSE_LEN;
                            }
                        }
                        else if (irsnd_buffer[current_bit / 8] & (1<<(7-(current_bit % 8))))
                        {
                            pulse_len = pulse_1_len;
                            pause_len = pause_1_len;
                        }
                        else
                        {
                            pulse_len = pulse_0_len;
                            pause_len = pause_0_len;
                        }
                    }
                    else if (has_stop_bit)                                                                      // send stop bit
                    {
                        pulse_len = pulse_0_len;

                        if (repetition_counter < n_repetitions)
                        {
                            pause_len = pause_0_len;
                        }
                        else
                        {
                            pause_len = 255;                                        // last frame: pause of 255
                        }
                    }

                    if (pulse_counter < pulse_len)
                    {
                        if (pulse_counter == 0)
                        {
                            irsnd_on ();
                        }
                        pulse_counter++;
                    }
                    else if (pause_counter < pause_len)
                    {
                        if (pause_counter == 0)
                        {
                            irsnd_off ();
                        }
                        pause_counter++;
                    }
                    else
                    {
                        current_bit++;

                        if (current_bit >= complete_data_len + has_stop_bit)
                        {
                            current_bit = 0xFF;
                            repetition_counter++;

                            if (repetition_counter == n_repetitions)
                            {
                                irsnd_busy = FALSE;
                                repetition_counter = 0;
                            }
                            new_frame = TRUE;
                        }

                        pulse_counter = 0;
                        pause_counter = 0;
                    }
                    break;
                }
#if IRSND_SUPPORT_RC5_PROTOCOL == 1
                case IRMP_RC5_PROTOCOL:
                {
                    uint8_t first_pulse;
                    uint8_t next_bit = FALSE;

                    if (current_bit == 0xFF)                                                    // 1 start bit
                    {
                        first_pulse = FALSE;
                    }
                    else                                                                        // send n'th bit
                    {
                        first_pulse = (irsnd_buffer[current_bit / 8] & (1<<(7-(current_bit % 8)))) ? FALSE : TRUE;
                    }

                    if (first_pulse)
                    {
                        if (pulse_counter < RC5_BIT_LEN)
                        {
                            if (pulse_counter == 0)
                            {
                                irsnd_on ();
                            }
                            pulse_counter++;
                        }
                        else if (pause_counter < RC5_BIT_LEN)
                        {
                            if (pause_counter == 0)
                            {
                                irsnd_off ();
                            }
                            pause_counter++;
                        }
                        else
                        {
                            next_bit = TRUE;
                        }
                    }
                    else
                    {
                        if (pause_counter < RC5_BIT_LEN)
                        {
                            if (pause_counter == 0)
                            {
                                irsnd_off ();
                            }
                            pause_counter++;
                        }
                        else if (pulse_counter < RC5_BIT_LEN)
                        {
                            if (pulse_counter == 0)
                            {
                                irsnd_on ();
                            }
                            pulse_counter++;
                        }
                        else
                        {
                            next_bit = TRUE;
                        }
                    }

                    if (next_bit)
                    {
                        current_bit++;

                        if (current_bit >= RC5_COMPLETE_DATA_LEN)
                        {
                            current_bit = 0xFF;
                            irsnd_busy  = FALSE;
                            new_frame = TRUE;
                            irsnd_off ();
                        }

                        pulse_counter = 0;
                        pause_counter = 0;
                    }
                    break;
                }
#endif // IRSND_SUPPORT_RC5_PROTOCOL
                default:
                {
                    irsnd_busy = FALSE;
                    break;
                }
            }
        }
    }

#ifdef DEBUG
    if (irsnd_is_on)
    {
        putchar ('0');
    }
    else
    {
        putchar ('1');
    }
#endif

    return irsnd_busy;
}

#ifdef DEBUG

// main function - for unix/linux + windows only!
// AVR: see main.c!
// Compile it under linux with:
// cc irsnd.c -o irsnd
//
// usage: ./irsnd protocol hex-address hex-command >filename

int
main (int argc, char ** argv)
{
    int         idx;
    int         cnt;
    int         protocol;
    int         address;
    int         command;
    int         repeat = 1;
    IRMP_DATA   irmp_data;

    if (argc != 4)
    {
        fprintf (stderr, "usage: %s protocol hex-address hex-command > filename\n", argv[0]);
        return 1;
    }

    if (sscanf (argv[1], "%d", &protocol) == 1 &&
        sscanf (argv[2], "%x", &address) == 1 &&
        sscanf (argv[3], "%x", &command) == 1)
    {
        irmp_data.protocol = protocol;
        irmp_data.address = address;
        irmp_data.command = command;

        irsnd_init ();

        for (cnt = 0; cnt < repeat; cnt++)
        {
            (void) irsnd_send_data (&irmp_data);

            for (idx = 0; idx < 3000; idx++)
            {
                irsnd_ISR ();
            }
        }

        putchar ('\n');
    }
    else
    {
        fprintf (stderr, "%s: wrong arguments\n", argv[0]);
        return 1;
    }
    return 0;
}

#endif // DEBUG
