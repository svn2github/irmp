/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndconfig.h
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irsndconfig.h,v 1.5 2010/06/02 13:25:22 fm Exp $
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
 * Change settings from 1 to 0 if you want to disable one or more encoders.
 * This saves program space.
 * 1 enable  decoder
 * 0 disable decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRSND_SUPPORT_SIRCS_PROTOCOL            1       // flag: support SIRCS                  uses ~150 bytes
#define IRSND_SUPPORT_NEC_PROTOCOL              1       // flag: support NEC + APPLE            uses ~100 bytes
#define IRSND_SUPPORT_SAMSUNG_PROTOCOL          1       // flag: support Samsung + Samsung32    uses ~300 bytes
#define IRSND_SUPPORT_MATSUSHITA_PROTOCOL       1       // flag: support Matsushita             uses ~150 bytes
#define IRSND_SUPPORT_KASEIKYO_PROTOCOL         0       // flag: support Kaseikyo               NOT SUPPORTED YET!
#define IRSND_SUPPORT_RECS80_PROTOCOL           1       // flag: support RECS80                 uses ~100 bytes
#define IRSND_SUPPORT_RC5_PROTOCOL              1       // flag: support RC5                    uses ~150 bytes
#define IRSND_SUPPORT_DENON_PROTOCOL            1       // flag: support DENON                  uses ~200 bytes
#define IRSND_SUPPORT_RC6_PROTOCOL              0       // flag: support RC6                    NOT SUPPORTED YET!
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        1       // flag: support RECS80EXT              uses ~100 bytes
#define IRSND_SUPPORT_NUBERT_PROTOCOL           1       // flag: support NUBERT                 uses ~100 bytes
#define IRSND_SUPPORT_BANG_OLUFSEN_PROTOCOL     1       // flag: support Bang&Olufsen           uses ~250 bytes
#define IRSND_SUPPORT_GRUNDIG_PROTOCOL          1       // flag: support Grundig                uses ~300 bytes
#define IRSND_SUPPORT_NOKIA_PROTOCOL            1       // flag: support Nokia                  uses ~400 bytes
#define IRSND_SUPPORT_SIEMENS_PROTOCOL          1       // flag: support Siemens, e.g. Gigaset  uses ~150 bytes

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

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second, F_INTERRUPTS should be in the range from 10000 to 15000
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define F_INTERRUPTS                            10000   // interrupts per second
