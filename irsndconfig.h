/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndconfig.h
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irsndconfig.h,v 1.12 2010/08/31 15:22:24 fm Exp $
 *
 * ATMEGA88 @ 8 MHz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second, F_INTERRUPTS should be in the range from 10000 to 15000
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#define F_INTERRUPTS                            10000   // interrupts per second
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more encoders.
 * This saves program space.
 * 1 enable  decoder
 * 0 disable decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
//      Protocol                                Enable  Remarks                 F_INTERRUPTS                Program Space
#define IRSND_SUPPORT_SIRCS_PROTOCOL            1       // Sony SIRCS           >= 10000                    uses ~150 bytes
#define IRSND_SUPPORT_NEC_PROTOCOL              1       // NEC + APPLE          >= 10000                    uses ~100 bytes
#define IRSND_SUPPORT_SAMSUNG_PROTOCOL          1       // Samsung + Samsung32  >= 10000                    uses ~300 bytes
#define IRSND_SUPPORT_MATSUSHITA_PROTOCOL       1       // Matsushita           >= 10000                    uses ~200 bytes
#define IRSND_SUPPORT_KASEIKYO_PROTOCOL         1       // Kaseikyo             >= 10000                    uses ~150 bytes
#define IRSND_SUPPORT_RC5_PROTOCOL              1       // RC5                  >= 10000                    uses ~150 bytes
#define IRSND_SUPPORT_DENON_PROTOCOL            1       // DENON                >= 10000                    uses ~200 bytes
#define IRSND_SUPPORT_JVC_PROTOCOL              1       // JVC                  >= 10000                    uses ~150 bytes
#define IRSND_SUPPORT_RC6_PROTOCOL              0       // RC6                  NOT SUPPORTED YET! DON'T CHANGE!
#define IRSND_SUPPORT_NUBERT_PROTOCOL           1       // NUBERT               >= 10000                    uses ~100 bytes
#define IRSND_SUPPORT_BANG_OLUFSEN_PROTOCOL     1       // Bang&Olufsen         >= 10000                    uses ~250 bytes
#define IRSND_SUPPORT_GRUNDIG_PROTOCOL          1       // Grundig              >= 10000                    uses ~300 bytes
#define IRSND_SUPPORT_NOKIA_PROTOCOL            1       // Nokia                >= 10000                    uses ~400 bytes
#define IRSND_SUPPORT_FDC_PROTOCOL              1       // FDC IR keyboard      >= 10000 (better 15000)     uses ~150 bytes
#define IRSND_SUPPORT_RCCAR_PROTOCOL            1       // RC CAR               >= 10000 (better 15000)     uses ~150 bytes
#define IRSND_SUPPORT_SIEMENS_PROTOCOL          0       // Siemens, Gigaset     >= 15000                    uses ~150 bytes
#define IRSND_SUPPORT_RECS80_PROTOCOL           0       // RECS80               >= 20000                    uses ~100 bytes
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        0       // RECS80EXT            >= 20000                    uses ~100 bytes


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega644P__)
#define IRSND_PORT                              PORTD   // port D
#define IRSND_DDR                               DDRD    // ddr D
#define IRSND_BIT                               7       // OC2A
#else
#define IRSND_PORT                              PORTB   // port B
#define IRSND_DDR                               DDRB    // ddr B
#define IRSND_BIT                               3       // OC2A
#endif // __AVR...


#if IRSND_SUPPORT_SIEMENS_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, SIEMENS protocol disabled (should be at least 15000)
#undef IRSND_SUPPORT_SIEMENS_PROTOCOL
#define IRSND_SUPPORT_SIEMENS_PROTOCOL          0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif

#if IRSND_SUPPORT_RECS80_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80 protocol disabled (should be at least 20000)
#undef IRSND_SUPPORT_RECS80_PROTOCOL
#define IRSND_SUPPORT_RECS80_PROTOCOL           0
#endif

#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80EXT protocol disabled (should be at least 20000)
#undef IRSND_SUPPORT_RECS80EXT_PROTOCOL
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        0
#endif
