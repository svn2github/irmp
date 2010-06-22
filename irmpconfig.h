/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmpconfig.h
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmpconfig.h,v 1.27 2010/06/22 12:39:51 fm Exp $
 *
 * ATMEGA88 @ 8 MHz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifndef _IRMPCONFIG_H_
#define _IRMPCONFIG_H_

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second, F_INTERRUPTS should be in the range from 10000 to 15000
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#define F_INTERRUPTS                            10000   // interrupts per second, min: 10000, max: 15000
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more decoders.
 * This saves program space.
 * 1 enable  decoder
 * 0 disable decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRMP_SUPPORT_SIRCS_PROTOCOL             1       // flag: support SIRCS                 uses ~100 bytes
#define IRMP_SUPPORT_NEC_PROTOCOL               1       // flag: support NEC + APPLE           uses ~250 bytes
#define IRMP_SUPPORT_SAMSUNG_PROTOCOL           1       // flag: support Samsung + Samsung32   uses ~250 bytes
#define IRMP_SUPPORT_MATSUSHITA_PROTOCOL        1       // flag: support Matsushita            uses  ~50 bytes
#define IRMP_SUPPORT_KASEIKYO_PROTOCOL          1       // flag: support Kaseikyo              uses  ~50 bytes
#define IRMP_SUPPORT_RECS80_PROTOCOL            1       // flag: support RECS80                uses  ~50 bytes
#define IRMP_SUPPORT_RC5_PROTOCOL               1       // flag: support RC5                   uses ~250 bytes
#define IRMP_SUPPORT_DENON_PROTOCOL             1       // flag: support DENON                 uses ~250 bytes
#define IRMP_SUPPORT_RC6_PROTOCOL               1       // flag: support RC6                   uses ~200 bytes
#define IRMP_SUPPORT_RECS80EXT_PROTOCOL         1       // flag: support RECS80EXT             uses  ~50 bytes
#define IRMP_SUPPORT_NUBERT_PROTOCOL            1       // flag: support NUBERT                uses  ~50 bytes
#define IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL      1       // flag: support Bang & Olufsen        uses ~200 bytes
#define IRMP_SUPPORT_GRUNDIG_PROTOCOL           1       // flag: support Grundig               uses ~150 bytes
#define IRMP_SUPPORT_NOKIA_PROTOCOL             1       // flag: support Nokia                 uses ~150 bytes
#define IRMP_SUPPORT_FDC_PROTOCOL               0       // flag: support FDC3402 keyboard      uses  ~50/400 bytes
#define IRMP_SUPPORT_RCCAR_PROTOCOL             0       // flag: support RC car                uses ~150/500 bytes

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * THE FOLLOWING DECODERS WORK ONLY FOR F_INTERRUPTS > 14500!
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if F_INTERRUPTS >= 14500
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           1       // flag: support Siemens Gigaset       uses ~150 bytes
#else
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef PIC_CCS_COMPILER                                 // PIC CCS Compiler:

#define IRMP_PIN                                PIN_B4  // use PB4 as IR input on PIC

#else                                                   // AVR:

#define IRMP_PORT                               PORTB
#define IRMP_DDR                                DDRB
#define IRMP_PIN                                PINB
#define IRMP_BIT                                6       // use PB6 as IR input on AVR

#define input(x)                                ((x) & (1 << IRMP_BIT))
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_LOGGING to 1 if want to log data to UART with 9600Bd
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_LOGGING
#define IRMP_LOGGING                            0       // 1: log IR signal (scan), 0: do not (default)
#endif

#endif /* _WC_IRMPCONFIG_H_ */
