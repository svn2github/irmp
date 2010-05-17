/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp.c - infrared multi-protocol decoder, supports several remote control protocols
 *
 * Copyright (c) 2009-2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmp.c,v 1.25 2010/05/17 10:31:43 fm Exp $
 *
 * ATMEGA88 @ 8 MHz
 *
 * Typical manufacturers:
 *
 * SIRCS      - Sony
 * NEC        - NEC, Yamaha, Canon, Tevion, Harman/Kardon, Hitachi, JVC, Pioneer, Toshiba, Xoro, Orion, and many other Japanese manufacturers
 * SAMSUNG    - Samsung
 * SAMSUNG32  - Samsung
 * MATSUSHITA - Matsushita
 * KASEIKYO   - Panasonic, Denon & other Japanese manufacturers (members of "Japan's Association for Electric Home Application")
 * RECS80     - Philips, Nokia, Thomson, Nordmende, Telefunken, Saba
 * RC5        - Philips and other European manufacturers
 * DENON      - Denon
 * RC6        - Philips and other European manufacturers
 * APPLE      - Apple
 * NUBERT     - Nubert Subwoofer System
 * B&O        - Bang & Olufsen
 * PANASONIC  - Panasonic (older, yet not implemented)
 * Grundig    - Grundig
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   SIRCS
 *   -----
 *
 *   frame: 1 start bit + 12-20 data bits + no stop bit
 *   data:  7 command bits + 5 address bits + 0 to 8 additional bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----------------_________           ------_____               ------------______
 *       2400us         600us             600us 600us               1200us      600 us        no stop bit
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   NEC + extended NEC
 *   -------------------------
 *
 *   frame: 1 start bit + 32 data bits + 1 stop bit
 *   data NEC:          8 address bits + 8 inverted address bits + 8 command bits + 8 inverted command bits
 *   data extended NEC: 16 address bits + 8 command bits + 8 inverted command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----------------_________           ------______              ------________________    ------______....
 *       9000us        4500us             560us  560us              560us    1690 us          560us
 *
 *
 *   Repetition frame:
 *
 *   -----------------_________------______  .... ~100ms Pause, then repeat
 *       9000us        2250us   560us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   SAMSUNG
 *   -------
 *
 *   frame: 1 start bit + 16 data(1) bits + 1 sync bit + additional 20 data(2) bits + 1 stop bit
 *   data(1): 16 address bits
 *   data(2): 4 ID bits + 8 command bits + 8 inverted command bits
 *
 *   start bit:                           data "0":                 data "1":                 sync bit:               stop bit:
 *   ----------______________             ------______              ------________________    ------______________    ------______....
 *    4500us       4500us                 550us  450us              550us    1450us           550us    4500us         550us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   SAMSUNG32
 *   ----------
 *
 *   frame: 1 start bit + 32 data bits + 1 stop bit
 *   data: 16 address bits + 16 command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   ----------______________             ------______              ------________________    ------______....
 *    4500us       4500us                 550us  450us              550us    1450us           550us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   MATSUSHITA
 *   ----------
 *
 *   frame: 1 start bit + 24 data bits + 1 stop bit
 *   data:  6 custom bits + 6 command bits + 12 address bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   ----------_________                  ------______              ------________________    ------______....
 *    3488us     3488us                   872us  872us              872us    2616us           872us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   KASEIKYO
 *   --------
 *
 *   frame: 1 start bit + 48 data bits + 1 stop bit
 *   data:  16 manufacturer bits + 4 parity bits + 4 genre1 bits + 4 genre2 bits + 10 command bits + 2 id bits + 8 parity bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   ----------______                     ------______              ------________________    ------______....
 *    3380us   1690us                     423us  423us              423us    1269us           423us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   RECS80
 *   ------
 *
 *   frame: 2 start bits + 10 data bits + 1 stop bit
 *   data:  1 toggle bit + 3 address bits + 6 command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----_____________________           -----____________         -----______________       ------_______....
 *   158us       7432us                   158us   4902us            158us    7432us           158us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   RECS80EXT
 *   ---------
 *
 *   frame: 2 start bits + 11 data bits + 1 stop bit
 *   data:  1 toggle bit + 4 address bits + 6 command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----_____________________           -----____________         -----______________       ------_______....
 *   158us       3637us                   158us   4902us            158us    7432us           158us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   RC5 + RC5X
 *   ----------
 *
 *   RC5 frame:  2 start bits + 12 data bits + no stop bit
 *   RC5 data:   1 toggle bit + 5 address bits + 6 command bits
 *   RC5X frame: 1 start bit +  13 data bits + no stop bit
 *   RC5X data:  1 inverted command bit + 1 toggle bit + 5 address bits + 6 command bits
 *
 *   start bit:              data "0":                data "1":
 *   ______-----             ------______             ______------
 *   889us 889us             889us 889us              889us 889us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   DENON
 *   -----
 *
 *   frame: 0 start bits + 16 data bits + stop bit + 65ms pause + 16 inverted data bits + stop bit
 *   data:  5 address bits + 10 command bits
 *
 *   data "0":                 data "1":
 *   ------________________    ------______________
 *   275us      1050us         275us   1900us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   RC6
 *   ---
 *
 *   RC6 frame:  1 start bit + 1 bit "1" + 3 mode bits + 1 toggle bit + 16 data bits + 2666 µs pause
 *   RC6 data:   8 address bits + 8 command bits
 *
 *   start  bit               toggle bit "0":      toggle bit "1":     data/mode "0":      data/mode "1":
 *   ____________-------      _______-------       -------_______      _______-------      -------_______
 *      2666us    889us        889us  889us         889us  889us        444us  444us        444us  444us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   APPLE
 *   -----
 *
 *   frame: 1 start bit + 32 data bits + 1 stop bit
 *   data:  16 address bits + 11100000 + 8 command bits
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *   -----------------_________           ------______              ------________________    ------______....
 *       9000us        4500us             560us  560us              560us    1690 us          560us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   NUBERT (subwoofer system)
 *   -------------------------
 *
 *   frame: 1 start bit + 10 data bits + 1 stop bit
 *   data:  0 address bits + 10 command bits ?
 *
 *   start bit:                       data "0":                 data "1":                 stop bit:
 *   ----------_____                  ------______              ------________________    ------______....
 *    1340us   340us                  500us 1300us              1340us 340us              500us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   BANG_OLUFSEN
 *   ------------
 *
 *   frame: 4 start bits + 16 data bits + 1 trailer bit + 1 stop bit
 *   data:  0 address bits + 16 command bits
 *
 *   1st start bit:  2nd start bit:      3rd start bit:       4th start bit:
 *   -----________   -----________       -----_____________   -----________
 *   210us 3000us    210us 3000us        210us   15000us      210us 3000us
 *
 *   data "0":       data "1":           data "repeat bit":   trailer bit:         stop bit:
 *   -----________   -----_____________  -----___________     -----_____________   -----____...
 *   210us 3000us    210us   9000us      210us   6000us       210us   12000us      210us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   Grundig
 *   -------
 *
 *   frame:  1 start packet + n info packets + 1 stop packet
 *   packet: 1 pre bit + 1 start bit + 9 data bits + no stop bit
 *   data of start packet:   9 x 1
 *   data of info  packet:   9 command bits
 *   data of stop  packet:   9 x 1
 *
 *   pre bit:              start bit           data "0":            data "1":
 *   ------____________    ------______        ______------         ------______             
 *   528us  2639us         528us  528us        528us  528us         528us  528us
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *   PANASONIC (older protocol, yet not implemented, see also MATSUSHITA, timing very similar)
 *   -----------------------------------------------------------------------------------------
 *
 *   frame: 1 start bit + 22 data bits + 1 stop bit
 *   22 data bits = 5 custom bits + 6 data bits + 5 inverted custom bits + 6 inverted data bits
 *
 *   European version:      T = 456us
 *   USA & Canada version:  T = 422us
 *
 *   start bit:                           data "0":                 data "1":                 stop bit:
 *        8T            8T                 2T   2T                   2T      6T                2T
 *   -------------____________            ------_____               ------_____________       ------_______....
 *      3648us        3648us              912us 912us               912us    2736us           912us                (Europe)
 *      3376us        3376us              844us 844us               844us    2532us           844us                (US)
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#if defined(__PCM__) || defined(__PCB__) || defined(__PCH__)                // CCS PIC Compiler instead of AVR
#define PIC_CCS_COMPILER
#endif

#ifdef unix                                                                 // test on linux/unix
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define DEBUG
#define PROGMEM
#define memcpy_P        memcpy

#else // not unix:

#ifdef WIN32
#include <stdio.h>
#include <string.h>
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
#define DEBUG
#define PROGMEM
#define memcpy_P        memcpy

#else

#ifndef CODEVISION

#ifdef PIC_CCS_COMPILER

#include <string.h>
typedef unsigned int8   uint8_t;
typedef unsigned int16  uint16_t;
#define PROGMEM
#define memcpy_P        memcpy

#else // AVR:

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#endif  // PIC_CCS_COMPILER
#endif  // CODEVISION

#endif // windows
#endif // unix

#include "irmp.h"
#include "irmpconfig.h"

#define IRMP_TIMEOUT_TIME                       16500.0e-6                    // timeout after 16.5 ms darkness
#define IRMP_TIMEOUT_LEN                        (uint8_t)(F_INTERRUPTS * IRMP_TIMEOUT_TIME + 0.5)
#define IRMP_REPETITION_TIME                    (uint16_t)(F_INTERRUPTS * 100.0e-3 + 0.5)  // autodetect key repetition within 100 msec

#define MIN_TOLERANCE_00                        1.0                           // -0%
#define MAX_TOLERANCE_00                        1.0                           // +0%

#define MIN_TOLERANCE_05                        0.95                          // -5%
#define MAX_TOLERANCE_05                        1.05                          // +5%

#define MIN_TOLERANCE_10                        0.9                           // -10%
#define MAX_TOLERANCE_10                        1.1                           // +10%

#define MIN_TOLERANCE_15                        0.85                          // -15%
#define MAX_TOLERANCE_15                        1.15                          // +15%

#define MIN_TOLERANCE_20                        0.8                           // -20%
#define MAX_TOLERANCE_20                        1.2                           // +20%

#define MIN_TOLERANCE_30                        0.7                           // -30%
#define MAX_TOLERANCE_30                        1.3                           // +30%

#define MIN_TOLERANCE_40                        0.6                           // -40%
#define MAX_TOLERANCE_40                        1.4                           // +40%

#define MIN_TOLERANCE_50                        0.5                           // -50%
#define MAX_TOLERANCE_50                        1.5                           // +50%

#define MIN_TOLERANCE_60                        0.4                           // -60%
#define MAX_TOLERANCE_60                        1.6                           // +60%

#define SIRCS_START_BIT_PULSE_LEN_MIN           ((uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SIRCS_START_BIT_PULSE_LEN_MAX           ((uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define SIRCS_START_BIT_PAUSE_LEN_MIN           ((uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define SIRCS_START_BIT_PAUSE_LEN_MAX           ((uint8_t)(F_INTERRUPTS * SIRCS_START_BIT_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1) // only 5% to avoid conflict with RC6
#define SIRCS_1_PULSE_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * SIRCS_1_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SIRCS_1_PULSE_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * SIRCS_1_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define SIRCS_0_PULSE_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * SIRCS_0_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SIRCS_0_PULSE_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * SIRCS_0_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define SIRCS_PAUSE_LEN_MIN                     ((uint8_t)(F_INTERRUPTS * SIRCS_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SIRCS_PAUSE_LEN_MAX                     ((uint8_t)(F_INTERRUPTS * SIRCS_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)

#define NEC_START_BIT_PULSE_LEN_MIN             ((uint8_t)(F_INTERRUPTS * NEC_START_BIT_PULSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_START_BIT_PULSE_LEN_MAX             ((uint8_t)(F_INTERRUPTS * NEC_START_BIT_PULSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define NEC_START_BIT_PAUSE_LEN_MIN             ((uint8_t)(F_INTERRUPTS * NEC_START_BIT_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_START_BIT_PAUSE_LEN_MAX             ((uint8_t)(F_INTERRUPTS * NEC_START_BIT_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define NEC_REPEAT_START_BIT_PAUSE_LEN_MIN      ((uint8_t)(F_INTERRUPTS * NEC_REPEAT_START_BIT_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_REPEAT_START_BIT_PAUSE_LEN_MAX      ((uint8_t)(F_INTERRUPTS * NEC_REPEAT_START_BIT_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define NEC_PULSE_LEN_MIN                       ((uint8_t)(F_INTERRUPTS * NEC_PULSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_PULSE_LEN_MAX                       ((uint8_t)(F_INTERRUPTS * NEC_PULSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define NEC_1_PAUSE_LEN_MIN                     ((uint8_t)(F_INTERRUPTS * NEC_1_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_1_PAUSE_LEN_MAX                     ((uint8_t)(F_INTERRUPTS * NEC_1_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define NEC_0_PAUSE_LEN_MIN                     ((uint8_t)(F_INTERRUPTS * NEC_0_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define NEC_0_PAUSE_LEN_MAX                     ((uint8_t)(F_INTERRUPTS * NEC_0_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)

#define SAMSUNG_START_BIT_PULSE_LEN_MIN         ((uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SAMSUNG_START_BIT_PULSE_LEN_MAX         ((uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define SAMSUNG_START_BIT_PAUSE_LEN_MIN         ((uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define SAMSUNG_START_BIT_PAUSE_LEN_MAX         ((uint8_t)(F_INTERRUPTS * SAMSUNG_START_BIT_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define SAMSUNG_PULSE_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * SAMSUNG_PULSE_TIME * MIN_TOLERANCE_30 + 0.5) - 1)
#define SAMSUNG_PULSE_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * SAMSUNG_PULSE_TIME * MAX_TOLERANCE_30 + 0.5) + 1)
#define SAMSUNG_1_PAUSE_LEN_MIN                 ((uint8_t)(F_INTERRUPTS * SAMSUNG_1_PAUSE_TIME * MIN_TOLERANCE_30 + 0.5) - 1)
#define SAMSUNG_1_PAUSE_LEN_MAX                 ((uint8_t)(F_INTERRUPTS * SAMSUNG_1_PAUSE_TIME * MAX_TOLERANCE_30 + 0.5) + 1)
#define SAMSUNG_0_PAUSE_LEN_MIN                 ((uint8_t)(F_INTERRUPTS * SAMSUNG_0_PAUSE_TIME * MIN_TOLERANCE_30 + 0.5) - 1)
#define SAMSUNG_0_PAUSE_LEN_MAX                 ((uint8_t)(F_INTERRUPTS * SAMSUNG_0_PAUSE_TIME * MAX_TOLERANCE_30 + 0.5) + 1)

#define MATSUSHITA_START_BIT_PULSE_LEN_MIN      ((uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PULSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define MATSUSHITA_START_BIT_PULSE_LEN_MAX      ((uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PULSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define MATSUSHITA_START_BIT_PAUSE_LEN_MIN      ((uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define MATSUSHITA_START_BIT_PAUSE_LEN_MAX      ((uint8_t)(F_INTERRUPTS * MATSUSHITA_START_BIT_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define MATSUSHITA_PULSE_LEN_MIN                ((uint8_t)(F_INTERRUPTS * MATSUSHITA_PULSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define MATSUSHITA_PULSE_LEN_MAX                ((uint8_t)(F_INTERRUPTS * MATSUSHITA_PULSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define MATSUSHITA_1_PAUSE_LEN_MIN              ((uint8_t)(F_INTERRUPTS * MATSUSHITA_1_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define MATSUSHITA_1_PAUSE_LEN_MAX              ((uint8_t)(F_INTERRUPTS * MATSUSHITA_1_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)
#define MATSUSHITA_0_PAUSE_LEN_MIN              ((uint8_t)(F_INTERRUPTS * MATSUSHITA_0_PAUSE_TIME * MIN_TOLERANCE_40 + 0.5) - 1)
#define MATSUSHITA_0_PAUSE_LEN_MAX              ((uint8_t)(F_INTERRUPTS * MATSUSHITA_0_PAUSE_TIME * MAX_TOLERANCE_40 + 0.5) + 1)

#define KASEIKYO_START_BIT_PULSE_LEN_MIN        ((uint8_t)(F_INTERRUPTS * KASEIKYO_START_BIT_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define KASEIKYO_START_BIT_PULSE_LEN_MAX        ((uint8_t)(F_INTERRUPTS * KASEIKYO_START_BIT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define KASEIKYO_START_BIT_PAUSE_LEN_MIN        ((uint8_t)(F_INTERRUPTS * KASEIKYO_START_BIT_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define KASEIKYO_START_BIT_PAUSE_LEN_MAX        ((uint8_t)(F_INTERRUPTS * KASEIKYO_START_BIT_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define KASEIKYO_PULSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * KASEIKYO_PULSE_TIME * MIN_TOLERANCE_50 + 0.5) - 1)
#define KASEIKYO_PULSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * KASEIKYO_PULSE_TIME * MAX_TOLERANCE_60 + 0.5) + 1)
#define KASEIKYO_1_PAUSE_LEN_MIN                ((uint8_t)(F_INTERRUPTS * KASEIKYO_1_PAUSE_TIME * MIN_TOLERANCE_50 + 0.5) - 1)
#define KASEIKYO_1_PAUSE_LEN_MAX                ((uint8_t)(F_INTERRUPTS * KASEIKYO_1_PAUSE_TIME * MAX_TOLERANCE_50 + 0.5) + 1)
#define KASEIKYO_0_PAUSE_LEN_MIN                ((uint8_t)(F_INTERRUPTS * KASEIKYO_0_PAUSE_TIME * MIN_TOLERANCE_50 + 0.5) - 1)
#define KASEIKYO_0_PAUSE_LEN_MAX                ((uint8_t)(F_INTERRUPTS * KASEIKYO_0_PAUSE_TIME * MAX_TOLERANCE_50 + 0.5) + 1)

#define RECS80_START_BIT_PULSE_LEN_MIN          ((uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PULSE_TIME * MIN_TOLERANCE_00 + 0.5) - 1)
#define RECS80_START_BIT_PULSE_LEN_MAX          ((uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80_START_BIT_PAUSE_LEN_MIN          ((uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80_START_BIT_PAUSE_LEN_MAX          ((uint8_t)(F_INTERRUPTS * RECS80_START_BIT_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80_PULSE_LEN_MIN                    ((uint8_t)(F_INTERRUPTS * RECS80_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80_PULSE_LEN_MAX                    ((uint8_t)(F_INTERRUPTS * RECS80_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80_1_PAUSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * RECS80_1_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80_1_PAUSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * RECS80_1_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80_0_PAUSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * RECS80_0_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80_0_PAUSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * RECS80_0_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)

#define RC5_START_BIT_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * RC5_BIT_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define RC5_START_BIT_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * RC5_BIT_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define RC5_BIT_LEN_MIN                         ((uint8_t)(F_INTERRUPTS * RC5_BIT_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define RC5_BIT_LEN_MAX                         ((uint8_t)(F_INTERRUPTS * RC5_BIT_TIME * MAX_TOLERANCE_20 + 0.5) + 1)

#define DENON_PULSE_LEN_MIN                     ((uint8_t)(F_INTERRUPTS * DENON_PULSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define DENON_PULSE_LEN_MAX                     ((uint8_t)(F_INTERRUPTS * DENON_PULSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define DENON_1_PAUSE_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * DENON_1_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define DENON_1_PAUSE_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * DENON_1_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define DENON_0_PAUSE_LEN_MIN                   ((uint8_t)(F_INTERRUPTS * DENON_0_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define DENON_0_PAUSE_LEN_MAX                   ((uint8_t)(F_INTERRUPTS * DENON_0_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)

#define RC6_START_BIT_PULSE_LEN_MIN             ((uint8_t)(F_INTERRUPTS * RC6_START_BIT_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RC6_START_BIT_PULSE_LEN_MAX             ((uint8_t)(F_INTERRUPTS * RC6_START_BIT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RC6_START_BIT_PAUSE_LEN_MIN             ((uint8_t)(F_INTERRUPTS * RC6_START_BIT_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RC6_START_BIT_PAUSE_LEN_MAX             ((uint8_t)(F_INTERRUPTS * RC6_START_BIT_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RC6_TOGGLE_BIT_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * RC6_TOGGLE_BIT_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RC6_TOGGLE_BIT_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * RC6_TOGGLE_BIT_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RC6_BIT_LEN_MIN                         ((uint8_t)(F_INTERRUPTS * RC6_BIT_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RC6_BIT_LEN_MAX                         ((uint8_t)(F_INTERRUPTS * RC6_BIT_TIME * MAX_TOLERANCE_10 + 0.5) + 1)

#define RECS80EXT_START_BIT_PULSE_LEN_MIN       ((uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PULSE_TIME * MIN_TOLERANCE_00 + 0.5) - 1)
#define RECS80EXT_START_BIT_PULSE_LEN_MAX       ((uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PULSE_TIME * MAX_TOLERANCE_00 + 0.5) + 1)
#define RECS80EXT_START_BIT_PAUSE_LEN_MIN       ((uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define RECS80EXT_START_BIT_PAUSE_LEN_MAX       ((uint8_t)(F_INTERRUPTS * RECS80EXT_START_BIT_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define RECS80EXT_PULSE_LEN_MIN                 ((uint8_t)(F_INTERRUPTS * RECS80EXT_PULSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80EXT_PULSE_LEN_MAX                 ((uint8_t)(F_INTERRUPTS * RECS80EXT_PULSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80EXT_1_PAUSE_LEN_MIN               ((uint8_t)(F_INTERRUPTS * RECS80EXT_1_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80EXT_1_PAUSE_LEN_MAX               ((uint8_t)(F_INTERRUPTS * RECS80EXT_1_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)
#define RECS80EXT_0_PAUSE_LEN_MIN               ((uint8_t)(F_INTERRUPTS * RECS80EXT_0_PAUSE_TIME * MIN_TOLERANCE_10 + 0.5) - 1)
#define RECS80EXT_0_PAUSE_LEN_MAX               ((uint8_t)(F_INTERRUPTS * RECS80EXT_0_PAUSE_TIME * MAX_TOLERANCE_10 + 0.5) + 1)

#define NUBERT_START_BIT_PULSE_LEN_MIN          ((uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PULSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_START_BIT_PULSE_LEN_MAX          ((uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PULSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define NUBERT_START_BIT_PAUSE_LEN_MIN          ((uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_START_BIT_PAUSE_LEN_MAX          ((uint8_t)(F_INTERRUPTS * NUBERT_START_BIT_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define NUBERT_1_PULSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * NUBERT_1_PULSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_1_PULSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * NUBERT_1_PULSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define NUBERT_1_PAUSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * NUBERT_1_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_1_PAUSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * NUBERT_1_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define NUBERT_0_PULSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * NUBERT_0_PULSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_0_PULSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * NUBERT_0_PULSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define NUBERT_0_PAUSE_LEN_MIN                  ((uint8_t)(F_INTERRUPTS * NUBERT_0_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define NUBERT_0_PAUSE_LEN_MAX                  ((uint8_t)(F_INTERRUPTS * NUBERT_0_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)

#define BANG_OLUFSEN_START_BIT1_PULSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT1_PULSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT1_PULSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT1_PULSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT1_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT1_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT2_PULSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT2_PULSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT2_PULSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT2_PULSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT2_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT2_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT3_PULSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT3_PULSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT3_PULSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT3_PULSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT3_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT3_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT4_PULSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT4_PULSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT4_PULSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT4_PULSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MIN   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT4_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MAX   ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_START_BIT4_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_PULSE_LEN_MIN              ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_PULSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_PULSE_LEN_MAX              ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_PULSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_1_PAUSE_LEN_MIN            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_1_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_1_PAUSE_LEN_MAX            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_1_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_0_PAUSE_LEN_MIN            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_0_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_0_PAUSE_LEN_MAX            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_0_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_R_PAUSE_LEN_MIN            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_R_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_R_PAUSE_LEN_MAX            ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_R_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)
#define BANG_OLUFSEN_TRAILER_BIT_PAUSE_LEN_MIN  ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_TRAILER_BIT_PAUSE_TIME * MIN_TOLERANCE_05 + 0.5) - 1)
#define BANG_OLUFSEN_TRAILER_BIT_PAUSE_LEN_MAX  ((uint8_t)(F_INTERRUPTS * BANG_OLUFSEN_TRAILER_BIT_PAUSE_TIME * MAX_TOLERANCE_05 + 0.5) + 1)

#define GRUNDIG_START_BIT_LEN_MIN               ((uint8_t)(F_INTERRUPTS * GRUNDIG_BIT_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define GRUNDIG_START_BIT_LEN_MAX               ((uint8_t)(F_INTERRUPTS * GRUNDIG_BIT_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define GRUNDIG_BIT_LEN_MIN                     ((uint8_t)(F_INTERRUPTS * GRUNDIG_BIT_TIME * MIN_TOLERANCE_20 + 0.5) - 1)
#define GRUNDIG_BIT_LEN_MAX                     ((uint8_t)(F_INTERRUPTS * GRUNDIG_BIT_TIME * MAX_TOLERANCE_20 + 0.5) + 1)
#define GRUNDIG_PRE_PAUSE_LEN_MIN               ((uint8_t)(F_INTERRUPTS * GRUNDIG_PRE_PAUSE_TIME * MIN_TOLERANCE_20 + 0.5) + 1)
#define GRUNDIG_PRE_PAUSE_LEN_MAX               ((uint8_t)(F_INTERRUPTS * GRUNDIG_PRE_PAUSE_TIME * MAX_TOLERANCE_20 + 0.5) + 1)

#define AUTO_REPETITION_LEN                     (uint16_t)(F_INTERRUPTS * AUTO_REPETITION_TIME + 0.5)       // use uint16_t!

#ifdef DEBUG
#define DEBUG_PUTCHAR(a)                        { if (! silent) { putchar (a);          } }
#define DEBUG_PRINTF(...)                       { if (! silent) { printf (__VA_ARGS__); } }
static int silent;
static int time_counter;
#else
#define DEBUG_PUTCHAR(a)
#define DEBUG_PRINTF(...)
#endif

#if IRMP_LOGGING == 1
#define irmp_logIsr(x)                          irmp_logIr((x) ? 1:0)
#define UART_BAUD                               9600L

// calculate real baud rate:
#define UBRR_VAL                                ((F_CPU + UART_BAUD * 8) / (UART_BAUD * 16) - 1)    // round
#define BAUD_REAL                               (F_CPU / (16 * (UBRR_VAL + 1)))                     // real baudrate

#ifdef CODEVISION
#if ((BAUD_REAL * 1000) / UART_BAUD - 1000) > 10
#  error Error of baud rate of RS232 UARTx is more than 1%. That is too high!
#endif

#else // not CODEVISION

#define BAUD_ERROR                              ((BAUD_REAL * 1000) / UART_BAUD - 1000)             // error in promille

#if ((BAUD_ERROR > 10) || (-BAUD_ERROR < 10))
#  error Error of baud rate of RS232 UARTx is more than 1%. That is too high!
#endif

#endif // CODEVISION

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initialize  UART
 *  @details  Initializes UART
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
irmp_uart_init (void)
{
    UCSR0B |= (1<<TXEN0);                                                                   // activate UART0 TX
    UBRR0H = UBRR_VAL >> 8;                                                                 // store baudrate (upper byte)
    UBRR0L = UBRR_VAL & 0xFF;                                                               // store baudrate (lower byte)
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Send character
 *  @details  Sends character
 *  @param    ch character to be transmitted
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
irmp_uart_putc (unsigned char ch)
{
    while (!(UCSR0A & (1<<UDRE0)))
    {
        ;
    }

    UDR0 = ch;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Log IR signal
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define c_startcycles                     2                                 // min count of zeros before start of logging
#define c_endBits                      1000                                 // log buffer size
#define c_datalen                       700                                 // number of sequenced highbits to detect end

static void
irmp_logIr (uint8_t val)
{
    static uint8_t  s_data[c_datalen];                                      // logging buffer
    static uint16_t s_dataIdx;                                              // number of written bits
    static uint8_t  s_startcycles;                                          // current number of start-zeros
    static uint16_t s_ctr;                                                  // counts sequenced highbits - to detect end

    if ((val == 0) && (s_startcycles < c_startcycles) && !s_dataIdx)        // prevent that single random zeros init logging
    {
        ++s_startcycles;
    }
    else
    {
        s_startcycles = 0;

        if (    (val == 0)                                                  // start or continue logging on "0"
            || ((val == 1) && (s_dataIdx != 0)))                            // "1" cannot init logging
        {
            if (val)
            {                                                               // set or clear bit in bitarray
                s_data[(s_dataIdx / 8)] |=  (1<<(s_dataIdx % 8));
            }
            else
            {
                s_data[(s_dataIdx / 8)] &= ~(1<<(s_dataIdx % 8));
            }

            ++s_dataIdx;

            if (val)
            {                                                               // if high received then look at log-stop condition
                ++s_ctr;

                if (s_ctr > c_endBits)
                {                                                           // if stop condition (200 sequenced ones) meets, output on uart
                    uint16_t i;

                    for (i = 0; i < c_startcycles; ++i)
                    {
                        irmp_uart_putc ('0');                               // the ignored starting zeros
                    }

                    for (i = 0;i < (s_dataIdx - c_endBits + 20) / 8; ++i)   // transform bitset into uart chars
                    {
                        uint8_t d = s_data[i];
                        uint8_t j;

                        for (j = 0;j<8;++j)
                        {
                            irmp_uart_putc ((d & 1) + '0');
                            d >>= 1;
                        }
                    }

                    irmp_uart_putc ('\n');
                    s_dataIdx = 0;
                }
            }
            else
            {
                s_ctr = 0;
            }
        }
    }
}

#else
#define irmp_logIsr(x)
#endif

typedef struct
{
    uint8_t    protocol;                                                // ir protocol
    uint8_t    pulse_1_len_min;                                         // minimum length of pulse with bit value 1
    uint8_t    pulse_1_len_max;                                         // maximum length of pulse with bit value 1
    uint8_t    pause_1_len_min;                                         // minimum length of pause with bit value 1
    uint8_t    pause_1_len_max;                                         // maximum length of pause with bit value 1
    uint8_t    pulse_0_len_min;                                         // minimum length of pulse with bit value 0
    uint8_t    pulse_0_len_max;                                         // maximum length of pulse with bit value 0
    uint8_t    pause_0_len_min;                                         // minimum length of pause with bit value 0
    uint8_t    pause_0_len_max;                                         // maximum length of pause with bit value 0
    uint8_t    address_offset;                                          // address offset
    uint8_t    address_end;                                             // end of address
    uint8_t    command_offset;                                          // command offset
    uint8_t    command_end;                                             // end of command
    uint8_t    complete_len;                                            // complete length of frame
    uint8_t    stop_bit;                                                // flag: frame has stop bit
    uint8_t    lsb_first;                                               // flag: LSB first
} IRMP_PARAMETER;

#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER sircs_param =
{
    IRMP_SIRCS_PROTOCOL,
    SIRCS_1_PULSE_LEN_MIN,
    SIRCS_1_PULSE_LEN_MAX,
    SIRCS_PAUSE_LEN_MIN,
    SIRCS_PAUSE_LEN_MAX,
    SIRCS_0_PULSE_LEN_MIN,
    SIRCS_0_PULSE_LEN_MAX,
    SIRCS_PAUSE_LEN_MIN,
    SIRCS_PAUSE_LEN_MAX,
    SIRCS_ADDRESS_OFFSET,
    SIRCS_ADDRESS_OFFSET + SIRCS_ADDRESS_LEN,
    SIRCS_COMMAND_OFFSET,
    SIRCS_COMMAND_OFFSET + SIRCS_COMMAND_LEN,
    SIRCS_COMPLETE_DATA_LEN,
    SIRCS_STOP_BIT,
    SIRCS_LSB
};

#endif

#if IRMP_SUPPORT_NEC_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER nec_param =
{
    IRMP_NEC_PROTOCOL,
    NEC_PULSE_LEN_MIN,
    NEC_PULSE_LEN_MAX,
    NEC_1_PAUSE_LEN_MIN,
    NEC_1_PAUSE_LEN_MAX,
    NEC_PULSE_LEN_MIN,
    NEC_PULSE_LEN_MAX,
    NEC_0_PAUSE_LEN_MIN,
    NEC_0_PAUSE_LEN_MAX,
    NEC_ADDRESS_OFFSET,
    NEC_ADDRESS_OFFSET + NEC_ADDRESS_LEN,
    NEC_COMMAND_OFFSET,
    NEC_COMMAND_OFFSET + NEC_COMMAND_LEN,
    NEC_COMPLETE_DATA_LEN,
    NEC_STOP_BIT,
    NEC_LSB
};

static PROGMEM IRMP_PARAMETER nec_rep_param =
{
    IRMP_NEC_PROTOCOL,
    NEC_PULSE_LEN_MIN,
    NEC_PULSE_LEN_MAX,
    NEC_1_PAUSE_LEN_MIN,
    NEC_1_PAUSE_LEN_MAX,
    NEC_PULSE_LEN_MIN,
    NEC_PULSE_LEN_MAX,
    NEC_0_PAUSE_LEN_MIN,
    NEC_0_PAUSE_LEN_MAX,
    0,
    0,
    0,
    0,
    0,
    NEC_STOP_BIT,
    NEC_LSB
};

#endif

#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER samsung_param =
{
    IRMP_SAMSUNG_PROTOCOL,
    SAMSUNG_PULSE_LEN_MIN,
    SAMSUNG_PULSE_LEN_MAX,
    SAMSUNG_1_PAUSE_LEN_MIN,
    SAMSUNG_1_PAUSE_LEN_MAX,
    SAMSUNG_PULSE_LEN_MIN,
    SAMSUNG_PULSE_LEN_MAX,
    SAMSUNG_0_PAUSE_LEN_MIN,
    SAMSUNG_0_PAUSE_LEN_MAX,
    SAMSUNG_ADDRESS_OFFSET,
    SAMSUNG_ADDRESS_OFFSET + SAMSUNG_ADDRESS_LEN,
    SAMSUNG_COMMAND_OFFSET,
    SAMSUNG_COMMAND_OFFSET + SAMSUNG_COMMAND_LEN,
    SAMSUNG_COMPLETE_DATA_LEN,
    SAMSUNG_STOP_BIT,
    SAMSUNG_LSB
};

#endif

#if IRMP_SUPPORT_MATSUSHITA_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER matsushita_param =
{
    IRMP_MATSUSHITA_PROTOCOL,
    MATSUSHITA_PULSE_LEN_MIN,
    MATSUSHITA_PULSE_LEN_MAX,
    MATSUSHITA_1_PAUSE_LEN_MIN,
    MATSUSHITA_1_PAUSE_LEN_MAX,
    MATSUSHITA_PULSE_LEN_MIN,
    MATSUSHITA_PULSE_LEN_MAX,
    MATSUSHITA_0_PAUSE_LEN_MIN,
    MATSUSHITA_0_PAUSE_LEN_MAX,
    MATSUSHITA_ADDRESS_OFFSET,
    MATSUSHITA_ADDRESS_OFFSET + MATSUSHITA_ADDRESS_LEN,
    MATSUSHITA_COMMAND_OFFSET,
    MATSUSHITA_COMMAND_OFFSET + MATSUSHITA_COMMAND_LEN,
    MATSUSHITA_COMPLETE_DATA_LEN,
    MATSUSHITA_STOP_BIT,
    MATSUSHITA_LSB
};

#endif

#if IRMP_SUPPORT_KASEIKYO_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER kaseikyo_param =
{
    IRMP_KASEIKYO_PROTOCOL,
    KASEIKYO_PULSE_LEN_MIN,
    KASEIKYO_PULSE_LEN_MAX,
    KASEIKYO_1_PAUSE_LEN_MIN,
    KASEIKYO_1_PAUSE_LEN_MAX,
    KASEIKYO_PULSE_LEN_MIN,
    KASEIKYO_PULSE_LEN_MAX,
    KASEIKYO_0_PAUSE_LEN_MIN,
    KASEIKYO_0_PAUSE_LEN_MAX,
    KASEIKYO_ADDRESS_OFFSET,
    KASEIKYO_ADDRESS_OFFSET + KASEIKYO_ADDRESS_LEN,
    KASEIKYO_COMMAND_OFFSET,
    KASEIKYO_COMMAND_OFFSET + KASEIKYO_COMMAND_LEN,
    KASEIKYO_COMPLETE_DATA_LEN,
    KASEIKYO_STOP_BIT,
    KASEIKYO_LSB
};

#endif

#if IRMP_SUPPORT_RECS80_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER recs80_param =
{
    IRMP_RECS80_PROTOCOL,
    RECS80_PULSE_LEN_MIN,
    RECS80_PULSE_LEN_MAX,
    RECS80_1_PAUSE_LEN_MIN,
    RECS80_1_PAUSE_LEN_MAX,
    RECS80_PULSE_LEN_MIN,
    RECS80_PULSE_LEN_MAX,
    RECS80_0_PAUSE_LEN_MIN,
    RECS80_0_PAUSE_LEN_MAX,
    RECS80_ADDRESS_OFFSET,
    RECS80_ADDRESS_OFFSET + RECS80_ADDRESS_LEN,
    RECS80_COMMAND_OFFSET,
    RECS80_COMMAND_OFFSET + RECS80_COMMAND_LEN,
    RECS80_COMPLETE_DATA_LEN,
    RECS80_STOP_BIT,
    RECS80_LSB
};

#endif

#if IRMP_SUPPORT_RC5_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER rc5_param =
{
    IRMP_RC5_PROTOCOL,
    RC5_BIT_LEN_MIN,
    RC5_BIT_LEN_MAX,
    RC5_BIT_LEN_MIN,
    RC5_BIT_LEN_MAX,
    1,                                // tricky: use this as stop bit length
    1,
    1,
    1,
    RC5_ADDRESS_OFFSET,
    RC5_ADDRESS_OFFSET + RC5_ADDRESS_LEN,
    RC5_COMMAND_OFFSET,
    RC5_COMMAND_OFFSET + RC5_COMMAND_LEN,
    RC5_COMPLETE_DATA_LEN,
    RC5_STOP_BIT,
    RC5_LSB
};

#endif

#if IRMP_SUPPORT_DENON_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER denon_param =
{
    IRMP_DENON_PROTOCOL,
    DENON_PULSE_LEN_MIN,
    DENON_PULSE_LEN_MAX,
    DENON_1_PAUSE_LEN_MIN,
    DENON_1_PAUSE_LEN_MAX,
    DENON_PULSE_LEN_MIN,
    DENON_PULSE_LEN_MAX,
    DENON_0_PAUSE_LEN_MIN,
    DENON_0_PAUSE_LEN_MAX,
    DENON_ADDRESS_OFFSET,
    DENON_ADDRESS_OFFSET + DENON_ADDRESS_LEN,
    DENON_COMMAND_OFFSET,
    DENON_COMMAND_OFFSET + DENON_COMMAND_LEN,
    DENON_COMPLETE_DATA_LEN,
    DENON_STOP_BIT,
    DENON_LSB
};

#endif

#if IRMP_SUPPORT_RC6_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER rc6_param =
{
    IRMP_RC6_PROTOCOL,
    RC6_BIT_LEN_MIN,
    RC6_BIT_LEN_MAX,
    RC6_BIT_LEN_MIN,
    RC6_BIT_LEN_MAX,
    1,                                // tricky: use this as stop bit length
    1,
    1,
    1,
    RC6_ADDRESS_OFFSET,
    RC6_ADDRESS_OFFSET + RC6_ADDRESS_LEN,
    RC6_COMMAND_OFFSET,
    RC6_COMMAND_OFFSET + RC6_COMMAND_LEN,
    RC6_COMPLETE_DATA_LEN_SHORT,
    RC6_STOP_BIT,
    RC6_LSB
};

#endif

#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER recs80ext_param =
{
    IRMP_RECS80EXT_PROTOCOL,
    RECS80EXT_PULSE_LEN_MIN,
    RECS80EXT_PULSE_LEN_MAX,
    RECS80EXT_1_PAUSE_LEN_MIN,
    RECS80EXT_1_PAUSE_LEN_MAX,
    RECS80EXT_PULSE_LEN_MIN,
    RECS80EXT_PULSE_LEN_MAX,
    RECS80EXT_0_PAUSE_LEN_MIN,
    RECS80EXT_0_PAUSE_LEN_MAX,
    RECS80EXT_ADDRESS_OFFSET,
    RECS80EXT_ADDRESS_OFFSET + RECS80EXT_ADDRESS_LEN,
    RECS80EXT_COMMAND_OFFSET,
    RECS80EXT_COMMAND_OFFSET + RECS80EXT_COMMAND_LEN,
    RECS80EXT_COMPLETE_DATA_LEN,
    RECS80EXT_STOP_BIT,
    RECS80EXT_LSB
};

#endif

#if IRMP_SUPPORT_NUBERT_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER nubert_param =
{
    IRMP_NUBERT_PROTOCOL,
    NUBERT_1_PULSE_LEN_MIN,
    NUBERT_1_PULSE_LEN_MAX,
    NUBERT_1_PAUSE_LEN_MIN,
    NUBERT_1_PAUSE_LEN_MAX,
    NUBERT_0_PULSE_LEN_MIN,
    NUBERT_0_PULSE_LEN_MAX,
    NUBERT_0_PAUSE_LEN_MIN,
    NUBERT_0_PAUSE_LEN_MAX,
    NUBERT_ADDRESS_OFFSET,
    NUBERT_ADDRESS_OFFSET + NUBERT_ADDRESS_LEN,
    NUBERT_COMMAND_OFFSET,
    NUBERT_COMMAND_OFFSET + NUBERT_COMMAND_LEN,
    NUBERT_COMPLETE_DATA_LEN,
    NUBERT_STOP_BIT,
    NUBERT_LSB
};

#endif

#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER bang_olufsen_param =
{
    IRMP_BANG_OLUFSEN_PROTOCOL,
    BANG_OLUFSEN_PULSE_LEN_MIN,
    BANG_OLUFSEN_PULSE_LEN_MAX,
    BANG_OLUFSEN_1_PAUSE_LEN_MIN,
    BANG_OLUFSEN_1_PAUSE_LEN_MAX,
    BANG_OLUFSEN_PULSE_LEN_MIN,
    BANG_OLUFSEN_PULSE_LEN_MAX,
    BANG_OLUFSEN_0_PAUSE_LEN_MIN,
    BANG_OLUFSEN_0_PAUSE_LEN_MAX,
    BANG_OLUFSEN_ADDRESS_OFFSET,
    BANG_OLUFSEN_ADDRESS_OFFSET + BANG_OLUFSEN_ADDRESS_LEN,
    BANG_OLUFSEN_COMMAND_OFFSET,
    BANG_OLUFSEN_COMMAND_OFFSET + BANG_OLUFSEN_COMMAND_LEN,
    BANG_OLUFSEN_COMPLETE_DATA_LEN,
    BANG_OLUFSEN_STOP_BIT,
    BANG_OLUFSEN_LSB
};

#endif

#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1

static PROGMEM IRMP_PARAMETER grundig_param =
{
    IRMP_GRUNDIG_PROTOCOL,
    GRUNDIG_BIT_LEN_MIN,
    GRUNDIG_BIT_LEN_MAX,
    GRUNDIG_BIT_LEN_MIN,
    GRUNDIG_BIT_LEN_MAX,
    1,                                // tricky: use this as stop bit length
    1,
    1,
    1,
    GRUNDIG_ADDRESS_OFFSET,
    GRUNDIG_ADDRESS_OFFSET + GRUNDIG_ADDRESS_LEN,
    GRUNDIG_COMMAND_OFFSET,
    GRUNDIG_COMMAND_OFFSET + GRUNDIG_COMMAND_LEN,
    GRUNDIG_COMPLETE_DATA_LEN,
    GRUNDIG_STOP_BIT,
    GRUNDIG_LSB
};

#endif

static uint8_t                              irmp_bit;                                           // current bit position
static IRMP_PARAMETER                       irmp_param;

static volatile uint8_t                     irmp_ir_detected;
static volatile uint8_t                     irmp_protocol;
static volatile uint16_t                    irmp_address;
static volatile uint16_t                    irmp_command;
static volatile uint16_t                    irmp_id;                                            // only used for SAMSUNG protocol
static volatile uint8_t                     irmp_flags;

#ifdef DEBUG
static uint8_t                              IRMP_PIN;
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Initialize IRMP decoder
 *  @details  Configures IRMP input pin
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef DEBUG
void
irmp_init (void)
{
#ifndef PIC_CCS_COMPILER
    IRMP_PORT &= ~(1<<IRMP_BIT);                                                                  // deactivate pullup
    IRMP_DDR &= ~(1<<IRMP_BIT);                                                                   // set pin to input
#endif // PIC_CCS_COMPILER

#if IRMP_LOGGING == 1
    irmp_uart_init ();
#endif
}
#endif
/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  Get IRMP data
 *  @details  gets decoded IRMP data
 *  @param    pointer in order to store IRMP data
 *  @return    TRUE: successful, FALSE: failed
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
irmp_get_data (IRMP_DATA * irmp_data_p)
{
    uint8_t   rtc = FALSE;

    if (irmp_ir_detected)
    {
        switch (irmp_protocol)
        {
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
            case IRMP_SAMSUNG_PROTOCOL:
                if ((irmp_command >> 8) == (~irmp_command & 0x00FF))
                {
                    irmp_command &= 0xff;
                    irmp_command |= irmp_id << 8;
                    rtc = TRUE;
                }
                break;
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
            case IRMP_NEC_PROTOCOL:
                if ((irmp_command >> 8) == (~irmp_command & 0x00FF))
                {
                    irmp_command &= 0xff;
                    rtc = TRUE;
                }
                else if ((irmp_command & 0xFF00) == 0xD100)
                {
                    DEBUG_PRINTF ("Switching to APPLE protocol\n");
                    irmp_protocol = IRMP_APPLE_PROTOCOL;
                    irmp_command &= 0xff;
                    rtc = TRUE;
                }
                break;
#endif
            default:
                rtc = TRUE;
        }

        if (rtc)
        {
            irmp_data_p->protocol = irmp_protocol;
            irmp_data_p->address = irmp_address;
            irmp_data_p->command = irmp_command;
            irmp_data_p->flags   = irmp_flags;
            irmp_command = 0;
            irmp_address = 0;
            irmp_flags   = 0;
        }

        irmp_ir_detected = FALSE;
    }

    return rtc;
}

// these statics must not be volatile, because they are only used by irmp_store_bit(), which is called by irmp_ISR()
static uint16_t   irmp_tmp_address;                                                         // ir address
static uint16_t   irmp_tmp_command;                                                         // ir command
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
static uint16_t   irmp_tmp_id;                                                              // ir id (only SAMSUNG)
#endif

static uint8_t    irmp_bit;                                                                 // current bit position

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  store bit
 *  @details  store bit in temp address or temp command
 *  @param    value to store: 0 or 1
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
irmp_store_bit (uint8_t value)
{
    if (irmp_bit >= irmp_param.address_offset && irmp_bit < irmp_param.address_end)
    {
        if (irmp_param.lsb_first)
        {
            irmp_tmp_address |= (((uint16_t) (value)) << (irmp_bit - irmp_param.address_offset));   // CV wants cast
        }
        else
        {
            irmp_tmp_address <<= 1;
            irmp_tmp_address |= value;
        }
    }
    else if (irmp_bit >= irmp_param.command_offset && irmp_bit < irmp_param.command_end)
    {
        if (irmp_param.lsb_first)
        {
            irmp_tmp_command |= (((uint16_t) (value)) << (irmp_bit - irmp_param.command_offset));   // CV wants cast
        }
        else
        {
            irmp_tmp_command <<= 1;
            irmp_tmp_command |= value;
        }
    }
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
    else if (irmp_param.protocol == IRMP_SAMSUNG_PROTOCOL && irmp_bit >= SAMSUNG_ID_OFFSET && irmp_bit < SAMSUNG_ID_OFFSET + SAMSUNG_ID_LEN)
    {
        irmp_tmp_id |= (((uint16_t) (value)) << (irmp_bit - SAMSUNG_ID_OFFSET));                    // store with LSB first
    }
#endif
    irmp_bit++;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *  ISR routine
 *  @details  ISR routine, called 10000 times per second
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
irmp_ISR (void)
{
    static uint8_t    irmp_start_bit_detected;                                  // flag: start bit detected
    static uint8_t    wait_for_space;                                           // flag: wait for data bit space
    static uint8_t    wait_for_start_space;                                     // flag: wait for start bit space
    static uint8_t    irmp_pulse_time;                                          // count bit time for pulse
    static uint8_t    irmp_pause_time;                                          // count bit time for pause
    static uint16_t   last_irmp_address = 0xFFFF;                               // save last irmp address to recognize key repetition
    static uint16_t   last_irmp_command = 0xFFFF;                               // save last irmp command to recognize key repetition
    static uint16_t   repetition_counter;                                       // SIRCS repeats frame 2-5 times with 45 ms pause
    static uint8_t    repetition_frame_number;
#if IRMP_SUPPORT_DENON_PROTOCOL == 1
    static uint16_t   last_irmp_denon_command;                                  // save last irmp command to recognize DENON frame repetition
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1
    static uint8_t    rc5_cmd_bit6;                                             // bit 6 of RC5 command is the inverted 2nd start bit
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1 || IRMP_SUPPORT_RC6_PROTOCOL == 1
    static uint8_t    last_pause;                                               // last pause value
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1 || IRMP_SUPPORT_RC6_PROTOCOL == 1 || IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
    static uint8_t    last_value;                                               // last bit value
#endif
    uint8_t           irmp_input;                                               // input value

#ifdef DEBUG
    time_counter++;
#endif

    irmp_input = input(IRMP_PIN);

    irmp_logIsr(irmp_input);                                                    // log ir signal, if IRMP_LOGGING defined

    if (! irmp_ir_detected)                                                     // ir code already detected?
    {                                                                           // no...
        if (! irmp_start_bit_detected)                                          // start bit detected?
        {                                                                       // no...
            if (!irmp_input)                                                    // receiving burst?
            {                                                                   // yes...
                irmp_pulse_time++;                                              // increment counter
            }
            else
            {                                                                   // no...
                if (irmp_pulse_time)                                            // it's dark....
                {                                                               // set flags for counting the time of darkness...
                    irmp_start_bit_detected = 1;
                    wait_for_start_space    = 1;
                    wait_for_space          = 0;
                    irmp_tmp_command        = 0;
                    irmp_tmp_address        = 0;
                    irmp_bit                = 0xff;
                    irmp_pause_time         = 1;                                // 1st pause: set to 1, not to 0!
#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                    rc5_cmd_bit6            = 0;                                // fm 2010-03-07: bugfix: reset it after incomplete RC5 frame!
#endif
                }
                else
                {
                    repetition_counter++;
                }
            }
        }
        else
        {
            if (wait_for_start_space)                                           // we have received start bit...
            {                                                                   // ...and are counting the time of darkness
                if (irmp_input)                                                 // still dark?
                {                                                               // yes
                    irmp_pause_time++;                                          // increment counter

                    if (irmp_pause_time > IRMP_TIMEOUT_LEN)                     // timeout?
                    {                                                           // yes...
                        DEBUG_PRINTF ("error 1: pause after start bit %d too long: %d\n", irmp_pulse_time, irmp_pause_time);
                        irmp_start_bit_detected = 0;                            // reset flags, let's wait for another start bit
                        irmp_pulse_time         = 0;
                        irmp_pause_time         = 0;
                    }
                }
                else
                {                                                               // receiving first data pulse!
                    IRMP_PARAMETER * irmp_param_p = (IRMP_PARAMETER *) 0;

                    DEBUG_PRINTF ("start-bit: pulse = %d, pause = %d\n", irmp_pulse_time, irmp_pause_time);

#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1
                    if (irmp_pulse_time >= SIRCS_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= SIRCS_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= SIRCS_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= SIRCS_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's SIRCS
                        DEBUG_PRINTF ("protocol = SIRCS, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        SIRCS_START_BIT_PULSE_LEN_MIN, SIRCS_START_BIT_PULSE_LEN_MAX,
                                        SIRCS_START_BIT_PAUSE_LEN_MIN, SIRCS_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) (IRMP_PARAMETER *) &sircs_param;
                    }
                    else
#endif // IRMP_SUPPORT_SIRCS_PROTOCOL == 1

#if IRMP_SUPPORT_NEC_PROTOCOL == 1
                    if (irmp_pulse_time >= NEC_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= NEC_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= NEC_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= NEC_START_BIT_PAUSE_LEN_MAX)
                    {
                        DEBUG_PRINTF ("protocol = NEC, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        NEC_START_BIT_PULSE_LEN_MIN, NEC_START_BIT_PULSE_LEN_MAX,
                                        NEC_START_BIT_PAUSE_LEN_MIN, NEC_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &nec_param;
                    }
                    else if (irmp_pulse_time >= NEC_START_BIT_PULSE_LEN_MIN        && irmp_pulse_time <= NEC_START_BIT_PULSE_LEN_MAX &&
                             irmp_pause_time >= NEC_REPEAT_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= NEC_REPEAT_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's NEC
                        DEBUG_PRINTF ("protocol = NEC (repetition frame), start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        NEC_START_BIT_PULSE_LEN_MIN, NEC_START_BIT_PULSE_LEN_MAX,
                                        NEC_REPEAT_START_BIT_PAUSE_LEN_MIN, NEC_REPEAT_START_BIT_PAUSE_LEN_MAX);

                        irmp_param_p = (IRMP_PARAMETER *) &nec_rep_param;
                    }
                    else
#endif // IRMP_SUPPORT_NEC_PROTOCOL == 1

#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
                    if (irmp_pulse_time >= SAMSUNG_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= SAMSUNG_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= SAMSUNG_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= SAMSUNG_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's SAMSUNG
                        DEBUG_PRINTF ("protocol = SAMSUNG, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        SAMSUNG_START_BIT_PULSE_LEN_MIN, SAMSUNG_START_BIT_PULSE_LEN_MAX,
                                        SAMSUNG_START_BIT_PAUSE_LEN_MIN, SAMSUNG_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &samsung_param;
                    }
                    else
#endif // IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1

#if IRMP_SUPPORT_MATSUSHITA_PROTOCOL == 1
                    if (irmp_pulse_time >= MATSUSHITA_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= MATSUSHITA_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= MATSUSHITA_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= MATSUSHITA_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's MATSUSHITA
                        DEBUG_PRINTF ("protocol = MATSUSHITA, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        MATSUSHITA_START_BIT_PULSE_LEN_MIN, MATSUSHITA_START_BIT_PULSE_LEN_MAX,
                                        MATSUSHITA_START_BIT_PAUSE_LEN_MIN, MATSUSHITA_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &matsushita_param;
                    }
                    else
#endif // IRMP_SUPPORT_MATSUSHITA_PROTOCOL == 1

#if IRMP_SUPPORT_KASEIKYO_PROTOCOL == 1
                    if (irmp_pulse_time >= KASEIKYO_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= KASEIKYO_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= KASEIKYO_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= KASEIKYO_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's KASEIKYO
                        DEBUG_PRINTF ("protocol = KASEIKYO, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        KASEIKYO_START_BIT_PULSE_LEN_MIN, KASEIKYO_START_BIT_PULSE_LEN_MAX,
                                        KASEIKYO_START_BIT_PAUSE_LEN_MIN, KASEIKYO_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &kaseikyo_param;
                    }
                    else
#endif // IRMP_SUPPORT_KASEIKYO_PROTOCOL == 1

#if IRMP_SUPPORT_RECS80_PROTOCOL == 1
                    if (irmp_pulse_time >= RECS80_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= RECS80_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= RECS80_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= RECS80_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's RECS80
                        DEBUG_PRINTF ("protocol = RECS80, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        RECS80_START_BIT_PULSE_LEN_MIN, RECS80_START_BIT_PULSE_LEN_MAX,
                                        RECS80_START_BIT_PAUSE_LEN_MIN, RECS80_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &recs80_param;
                    }
                    else
#endif // IRMP_SUPPORT_RECS80_PROTOCOL == 1

#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                    if (((irmp_pulse_time >= RC5_START_BIT_LEN_MIN && irmp_pulse_time <= RC5_START_BIT_LEN_MAX) ||
                         (irmp_pulse_time >= 2 * RC5_START_BIT_LEN_MIN && irmp_pulse_time <= 2 * RC5_START_BIT_LEN_MAX)) &&
                        ((irmp_pause_time >= RC5_START_BIT_LEN_MIN && irmp_pause_time <= RC5_START_BIT_LEN_MAX) ||
                         (irmp_pause_time >= 2 * RC5_START_BIT_LEN_MIN && irmp_pause_time <= 2 * RC5_START_BIT_LEN_MAX)))
                    {                                                           // it's RC5
                        DEBUG_PRINTF ("protocol = RC5, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        RC5_START_BIT_LEN_MIN, RC5_START_BIT_LEN_MAX,
                                        RC5_START_BIT_LEN_MIN, RC5_START_BIT_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &rc5_param;
                        last_pause = irmp_pause_time;

                        if ((irmp_pulse_time > RC5_START_BIT_LEN_MAX && irmp_pulse_time <= 2 * RC5_START_BIT_LEN_MAX) ||
                            (irmp_pause_time > RC5_START_BIT_LEN_MAX && irmp_pause_time <= 2 * RC5_START_BIT_LEN_MAX))
                        {
                          last_value  = 0;
                          rc5_cmd_bit6 = 1<<6;
                        }
                        else
                        {
                          last_value  = 1;
                        }
                    }
                    else
#endif // IRMP_SUPPORT_RC5_PROTOCOL == 1

#if IRMP_SUPPORT_DENON_PROTOCOL == 1
                    if ( (irmp_pulse_time >= DENON_PULSE_LEN_MIN && irmp_pulse_time <= DENON_PULSE_LEN_MAX) &&
                        ((irmp_pause_time >= DENON_1_PAUSE_LEN_MIN && irmp_pause_time <= DENON_1_PAUSE_LEN_MAX) ||
                         (irmp_pause_time >= DENON_0_PAUSE_LEN_MIN && irmp_pause_time <= DENON_0_PAUSE_LEN_MAX)))
                    {                                                           // it's DENON
                        DEBUG_PRINTF ("protocol = DENON, start bit timings: pulse: %3d - %3d, pause: %3d - %3d or %3d - %3d\n",
                                        DENON_PULSE_LEN_MIN, DENON_PULSE_LEN_MAX,
                                        DENON_1_PAUSE_LEN_MIN, DENON_1_PAUSE_LEN_MAX,
                                        DENON_0_PAUSE_LEN_MIN, DENON_0_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &denon_param;
                    }
                    else
#endif // IRMP_SUPPORT_DENON_PROTOCOL == 1

#if IRMP_SUPPORT_RC6_PROTOCOL == 1
                    if (irmp_pulse_time >= RC6_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= RC6_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= RC6_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= RC6_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's RC6
                        DEBUG_PRINTF ("protocol = RC6, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        RC6_START_BIT_PULSE_LEN_MIN, RC6_START_BIT_PULSE_LEN_MAX,
                                        RC6_START_BIT_PAUSE_LEN_MIN, RC6_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &rc6_param;
                        last_pause = 0;
                        last_value = 0;
                    }
                    else
#endif // IRMP_SUPPORT_RC6_PROTOCOL == 1

#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1
                    if (irmp_pulse_time >= RECS80EXT_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= RECS80EXT_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= RECS80EXT_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= RECS80EXT_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's RECS80EXT
                        DEBUG_PRINTF ("protocol = RECS80EXT, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        RECS80EXT_START_BIT_PULSE_LEN_MIN, RECS80EXT_START_BIT_PULSE_LEN_MAX,
                                        RECS80EXT_START_BIT_PAUSE_LEN_MIN, RECS80EXT_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &recs80ext_param;
                    }
                    else
#endif // IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1

#if IRMP_SUPPORT_NUBERT_PROTOCOL == 1
                    if (irmp_pulse_time >= NUBERT_START_BIT_PULSE_LEN_MIN && irmp_pulse_time <= NUBERT_START_BIT_PULSE_LEN_MAX &&
                        irmp_pause_time >= NUBERT_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= NUBERT_START_BIT_PAUSE_LEN_MAX)
                    {                                                           // it's NUBERT
                        DEBUG_PRINTF ("protocol = NUBERT, start bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        NUBERT_START_BIT_PULSE_LEN_MIN, NUBERT_START_BIT_PULSE_LEN_MAX,
                                        NUBERT_START_BIT_PAUSE_LEN_MIN, NUBERT_START_BIT_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &nubert_param;
                    }
                    else
#endif // IRMP_SUPPORT_NUBERT_PROTOCOL == 1

#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
                    if (irmp_pulse_time >= BANG_OLUFSEN_START_BIT1_PULSE_LEN_MIN && irmp_pulse_time <= BANG_OLUFSEN_START_BIT1_PULSE_LEN_MAX &&
                        irmp_pause_time >= BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MAX)
                    {                                                           // it's BANG_OLUFSEN
                        DEBUG_PRINTF ("protocol = BANG_OLUFSEN\n");
                        DEBUG_PRINTF ("start bit 1 timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        BANG_OLUFSEN_START_BIT1_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT1_PULSE_LEN_MAX,
                                        BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MAX);
                        DEBUG_PRINTF ("start bit 2 timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        BANG_OLUFSEN_START_BIT2_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT2_PULSE_LEN_MAX,
                                        BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MAX);
                        DEBUG_PRINTF ("start bit 3 timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        BANG_OLUFSEN_START_BIT3_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT3_PULSE_LEN_MAX,
                                        BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MAX);
                        DEBUG_PRINTF ("start bit 4 timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        BANG_OLUFSEN_START_BIT4_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT4_PULSE_LEN_MAX,
                                        BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &bang_olufsen_param;
                        last_value = 0;
                    }
                    else
#endif // IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1

#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
                    if (irmp_pulse_time >= GRUNDIG_START_BIT_LEN_MIN && irmp_pulse_time <= GRUNDIG_START_BIT_LEN_MAX &&
                        irmp_pause_time >= GRUNDIG_PRE_PAUSE_LEN_MIN && irmp_pause_time <= GRUNDIG_PRE_PAUSE_LEN_MAX)
                    {                                                           // it's GRUNDIG
                        DEBUG_PRINTF ("protocol = GRUNDIG, pre bit timings: pulse: %3d - %3d, pause: %3d - %3d\n",
                                        GRUNDIG_START_BIT_LEN_MIN, GRUNDIG_START_BIT_LEN_MAX,
                                        GRUNDIG_PRE_PAUSE_LEN_MIN, GRUNDIG_PRE_PAUSE_LEN_MAX);
                        irmp_param_p = (IRMP_PARAMETER *) &grundig_param;
                        last_pause = irmp_pause_time;
                        last_value  = 1;
                    }
                    else
#endif // IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1

                    {
                        DEBUG_PRINTF ("protocol = UNKNOWN\n");
                        irmp_start_bit_detected = 0;                            // wait for another start bit...
                    }

                    if (irmp_start_bit_detected)
                    {
                        memcpy_P (&irmp_param, irmp_param_p, sizeof (IRMP_PARAMETER));

                        DEBUG_PRINTF ("pulse_1: %3d - %3d\n", irmp_param.pulse_1_len_min, irmp_param.pulse_1_len_max);
                        DEBUG_PRINTF ("pause_1: %3d - %3d\n", irmp_param.pause_1_len_min, irmp_param.pause_1_len_max);

#if IRMP_SUPPORT_RC6_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_RC6_PROTOCOL)
                        {
                            DEBUG_PRINTF ("pulse_toggle: %3d - %3d\n", RC6_TOGGLE_BIT_LEN_MIN, RC6_TOGGLE_BIT_LEN_MAX);
                        }
#endif
                        DEBUG_PRINTF ("pulse_0: %3d - %3d\n", irmp_param.pulse_0_len_min, irmp_param.pulse_0_len_max);
                        DEBUG_PRINTF ("pause_0: %3d - %3d\n", irmp_param.pause_0_len_min, irmp_param.pause_0_len_max);

#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_BANG_OLUFSEN_PROTOCOL)
                        {
                            DEBUG_PRINTF ("pulse_r: %3d - %3d\n", irmp_param.pulse_0_len_min, irmp_param.pulse_0_len_max);
                            DEBUG_PRINTF ("pause_r: %3d - %3d\n", BANG_OLUFSEN_R_PAUSE_LEN_MIN, BANG_OLUFSEN_R_PAUSE_LEN_MAX);
                        }
#endif

                        DEBUG_PRINTF ("command_offset: %2d\n", irmp_param.command_offset);
                        DEBUG_PRINTF ("command_len:    %3d\n", irmp_param.command_end - irmp_param.command_offset);
                        DEBUG_PRINTF ("complete_len:   %3d\n", irmp_param.complete_len);
                        DEBUG_PRINTF ("stop_bit:       %3d\n", irmp_param.stop_bit);
                    }

                    irmp_bit = 0;

#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_RC5_PROTOCOL)
                    {
                        if (irmp_pause_time > RC5_START_BIT_LEN_MAX && irmp_pause_time <= 2 * RC5_START_BIT_LEN_MAX)
                        {
                          DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);
                          DEBUG_PUTCHAR ('1');
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (1);
                        }
                        else if (! last_value)
                        {
                          DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);
                          DEBUG_PUTCHAR ('0');
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (0);
                        }
                    }
                    else
#endif // IRMP_SUPPORT_RC5_PROTOCOL == 1

#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_GRUNDIG_PROTOCOL)
                    {
                        if (irmp_pause_time > GRUNDIG_START_BIT_LEN_MAX && irmp_pause_time <= 2 * GRUNDIG_START_BIT_LEN_MAX)
                        {
                          DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);
                          DEBUG_PUTCHAR ('0');
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (0);
                        }
                        else if (! last_value)
                        {
                          DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);
                          DEBUG_PUTCHAR ('1');
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (1);
                        }
                    }
                    else
#endif // IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1

#if IRMP_SUPPORT_DENON_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_DENON_PROTOCOL)
                    {
                        DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);

                        if (irmp_pause_time >= DENON_1_PAUSE_LEN_MIN && irmp_pause_time <= DENON_1_PAUSE_LEN_MAX)
                        {                                                       // pause timings correct for "1"?
                          DEBUG_PUTCHAR ('1');                                  // yes, store 1
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (1);
                        }
                        else // if (irmp_pause_time >= DENON_0_PAUSE_LEN_MIN && irmp_pause_time <= DENON_0_PAUSE_LEN_MAX)
                        {                                                       // pause timings correct for "0"?
                          DEBUG_PUTCHAR ('0');                                  // yes, store 0
                          DEBUG_PUTCHAR ('\n');
                          irmp_store_bit (0);
                        }
                    }
#endif // IRMP_SUPPORT_DENON_PROTOCOL == 1

                    irmp_pulse_time = 1;                                        // set counter to 1, not 0
                    irmp_pause_time = 0;
                    wait_for_start_space = 0;
                }
            }
            else if (wait_for_space)                                            // the data section....
            {                                                                   // counting the time of darkness....
                uint8_t got_light = FALSE;

                if (irmp_input)                                                 // still dark?
                {                                                               // yes...
                    if (irmp_bit == irmp_param.complete_len && irmp_param.stop_bit == 1)
                    {
                        if (irmp_pulse_time >= irmp_param.pulse_0_len_min && irmp_pulse_time <= irmp_param.pulse_0_len_max)
                        {
#ifdef DEBUG
                            if (irmp_param.protocol != IRMP_RC5_PROTOCOL)
                            {
                                DEBUG_PRINTF ("stop bit detected\n");
                            }
#endif
                            irmp_param.stop_bit = 0;
                        }
                        else
                        {
                            DEBUG_PRINTF ("stop bit timing wrong\n");

                            irmp_start_bit_detected = 0;                        // wait for another start bit...
                            irmp_pulse_time         = 0;
                            irmp_pause_time         = 0;
                        }
                    }
                    else
                    {
                        irmp_pause_time++;                                      // increment counter

#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_SIRCS_PROTOCOL &&       // Sony has a variable number of bits:
                            irmp_pause_time > SIRCS_PAUSE_LEN_MAX &&            // minimum is 12
                            irmp_bit >= 12 - 1)                                 // pause too long?
                        {                                                       // yes, break and close this frame
                            irmp_param.complete_len = irmp_bit + 1;             // set new complete length
                            got_light = TRUE;                                   // this is a lie, but helps (generates stop bit)
                            irmp_param.command_end = irmp_param.command_offset + irmp_bit + 1;        // correct command length
                            irmp_pause_time = SIRCS_PAUSE_LEN_MAX - 1;          // correct pause length
                        }
                        else
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_RC5_PROTOCOL &&
                            irmp_pause_time > 2 * RC5_BIT_LEN_MAX && irmp_bit >= RC5_COMPLETE_DATA_LEN - 2 && !irmp_param.stop_bit)
                        {                                                       // special rc5 decoder
                            got_light = TRUE;                                   // this is a lie, but generates a stop bit ;-)
                            irmp_param.stop_bit = TRUE;                         // set flag
                        }
                        else
#endif
#if IRMP_SUPPORT_RC6_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_RC6_PROTOCOL &&
                            irmp_pause_time > 2 * RC6_BIT_LEN_MAX && irmp_bit >= irmp_param.complete_len - 2 && !irmp_param.stop_bit)
                        {                                                       // special rc6 decoder
                            got_light = TRUE;                                   // this is a lie, but generates a stop bit ;-)
                            irmp_param.stop_bit = TRUE;                         // set flag
                        }
                        else
#endif
#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_GRUNDIG_PROTOCOL &&
                            irmp_pause_time > 2 * GRUNDIG_BIT_LEN_MAX && irmp_bit >= GRUNDIG_COMPLETE_DATA_LEN - 2 && !irmp_param.stop_bit)
                        {                                                       // special rc5 decoder
                            got_light = TRUE;                                   // this is a lie, but generates a stop bit ;-)
                            irmp_param.stop_bit = TRUE;                         // set flag
                        }
                        else
#endif
                        if (irmp_pause_time > IRMP_TIMEOUT_LEN)                 // timeout?
                        {                                                       // yes...
                            if (irmp_bit == irmp_param.complete_len - 1 && irmp_param.stop_bit == 0)
                            {
                                irmp_bit++;
                            }
                            else
                            {
                                DEBUG_PRINTF ("error 2: pause %d after data bit %d too long\n", irmp_pause_time, irmp_bit);

                                irmp_start_bit_detected = 0;                    // wait for another start bit...
                                irmp_pulse_time         = 0;
                                irmp_pause_time         = 0;
                            }
                        }
                    }
                }
                else
                {                                                               // got light now!
                    got_light = TRUE;
                }

                if (got_light)
                {
                    DEBUG_PRINTF ("%8d [bit %2d: pulse = %3d, pause = %3d] ", time_counter, irmp_bit, irmp_pulse_time, irmp_pause_time);

#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_RC5_PROTOCOL)               // special rc5 decoder
                    {
                        if (irmp_pulse_time > RC5_BIT_LEN_MAX && irmp_pulse_time <= 2 * RC5_BIT_LEN_MAX)
                        {
                            DEBUG_PUTCHAR ('1');
                            irmp_store_bit (1);
                            DEBUG_PUTCHAR ('0');
                            DEBUG_PUTCHAR ('\n');
                            irmp_store_bit (0);
                            last_value = 0;
                        }

                        else // if (irmp_pulse_time >= RC5_BIT_LEN_MIN && irmp_pulse_time <= RC5_BIT_LEN_MAX)
                        {
                            uint8_t rc5_value;

                            if (last_pause > RC5_BIT_LEN_MAX && last_pause <= 2 * RC5_BIT_LEN_MAX)
                            {
                                rc5_value = last_value ? 0 : 1;
                                last_value  = rc5_value;
                            }
                            else
                            {
                                rc5_value = last_value;
                            }

                            DEBUG_PUTCHAR (rc5_value + '0');
                            DEBUG_PUTCHAR ('\n');
                            irmp_store_bit (rc5_value);
                        }

                        last_pause = irmp_pause_time;
                        wait_for_space = 0;
                    }
                    else
#endif

#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_GRUNDIG_PROTOCOL)               // special Grundig decoder
                    {
                        if (irmp_pulse_time > GRUNDIG_BIT_LEN_MAX && irmp_pulse_time <= 2 * GRUNDIG_BIT_LEN_MAX)
                        {
                            DEBUG_PUTCHAR ('0');
                            irmp_store_bit (0);
                            DEBUG_PUTCHAR ('1');
                            DEBUG_PUTCHAR ('\n');
                            irmp_store_bit (1);
                            last_value = 1;
                        }

                        else // if (irmp_pulse_time >= GRUNDIG_BIT_LEN_MIN && irmp_pulse_time <= GRUNDIG_BIT_LEN_MAX)
                        {
                            uint8_t grundig_value;

                            if (last_pause > GRUNDIG_BIT_LEN_MAX && last_pause <= 2 * GRUNDIG_BIT_LEN_MAX)
                            {
                                grundig_value = last_value ? 0 : 1;
                                last_value  = grundig_value;
                            }
                            else
                            {
                                grundig_value = last_value;
                            }

                            DEBUG_PUTCHAR (grundig_value + '0');
                            DEBUG_PUTCHAR ('\n');
                            irmp_store_bit (grundig_value);
                        }

                        last_pause = irmp_pause_time;
                        wait_for_space = 0;
                    }
                    else
#endif

#if IRMP_SUPPORT_RC6_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_RC6_PROTOCOL)               // special rc6 decoder
                    {
                        switch (irmp_bit)
                        {                                                       // handle toggle bit, which is 2 times longer than other bits
                            case 3:
                            case 4:
                            case 5:
                                if (irmp_pulse_time > RC6_TOGGLE_BIT_LEN_MAX && irmp_pause_time > RC6_TOGGLE_BIT_LEN_MAX)
                                {
                                    DEBUG_PUTCHAR ('1');
                                    irmp_store_bit (1);
                                }

                                DEBUG_PUTCHAR ('0');
                                irmp_store_bit (0);
                                last_value = 0;
                                DEBUG_PUTCHAR ('\n');
                                break;

                            default:
                                if (irmp_pulse_time > RC6_BIT_LEN_MAX && irmp_pulse_time <= 2 * RC6_BIT_LEN_MAX)
                                {
                                    DEBUG_PUTCHAR ('0');
                                    irmp_store_bit (0);
                                    DEBUG_PUTCHAR ('1');
                                    DEBUG_PUTCHAR ('\n');
                                    irmp_store_bit (1);
                                    last_value = 1;
                                }
                                else // if (irmp_pulse_time >= RC6_BIT_LEN_MIN && irmp_pulse_time <= RC6_BIT_LEN_MAX)
                                {
                                    uint8_t rc5_value;

                                    if (last_pause > RC6_BIT_LEN_MAX && last_pause <= 2 * RC6_BIT_LEN_MAX)
                                    {
                                        rc5_value = last_value ? 0 : 1;
                                        last_value  = rc5_value;
                                    }
                                    else
                                    {
                                        rc5_value = last_value;
                                    }

                                    if (irmp_bit == 1 && rc5_value == 0)
                                    {
                                        irmp_param.complete_len = RC6_COMPLETE_DATA_LEN_LONG;
                                    }

                                    DEBUG_PUTCHAR (rc5_value + '0');
                                    DEBUG_PUTCHAR ('\n');
                                    irmp_store_bit (rc5_value);
                                }

                                last_pause = irmp_pause_time;
                                break;
                        } // switch

                        wait_for_space = 0;
                    }
                    else
#endif // IRMP_SUPPORT_RC6_PROTOCOL == 1

#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_SAMSUNG_PROTOCOL && irmp_bit == 16)       // Samsung: 16th bit
                    {
                        if (irmp_pulse_time >= SAMSUNG_PULSE_LEN_MIN && irmp_pulse_time <= SAMSUNG_PULSE_LEN_MAX &&
                            irmp_pause_time >= SAMSUNG_START_BIT_PAUSE_LEN_MIN && irmp_pause_time <= SAMSUNG_START_BIT_PAUSE_LEN_MAX)
                        {
                            DEBUG_PRINTF ("SYNC\n");
                            wait_for_space = 0;
                            irmp_tmp_id = 0;
                            irmp_bit++;
                        }
                        else  if (irmp_pulse_time >= SAMSUNG_PULSE_LEN_MIN && irmp_pulse_time <= SAMSUNG_PULSE_LEN_MAX)
                        {
                            irmp_param.protocol         = IRMP_SAMSUNG32_PROTOCOL;
                            irmp_param.command_offset   = SAMSUNG32_COMMAND_OFFSET;
                            irmp_param.command_end      = SAMSUNG32_COMMAND_OFFSET + SAMSUNG32_COMMAND_LEN;
                            irmp_param.complete_len     = SAMSUNG32_COMPLETE_DATA_LEN;

                            if (irmp_pause_time >= SAMSUNG_1_PAUSE_LEN_MIN && irmp_pause_time <= SAMSUNG_1_PAUSE_LEN_MAX)
                            {
                                DEBUG_PUTCHAR ('1');
                                DEBUG_PUTCHAR ('\n');
                                irmp_store_bit (1);
                                wait_for_space = 0;
                            }
                            else
                            {
                                DEBUG_PUTCHAR ('0');
                                DEBUG_PUTCHAR ('\n');
                                irmp_store_bit (0);
                                wait_for_space = 0;
                            }

                            DEBUG_PRINTF ("Switching to SAMSUNG32 protocol\n");
                        }
                        else
                        {                                                           // timing incorrect!
                            DEBUG_PRINTF ("error 3 Samsung: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                            irmp_start_bit_detected = 0;                            // reset flags and wait for next start bit
                            irmp_pause_time         = 0;
                        }
                    }
                    else
#endif // IRMP_SUPPORT_SAMSUNG_PROTOCOL

#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_BANG_OLUFSEN_PROTOCOL)
                    {
                        if (irmp_pulse_time >= BANG_OLUFSEN_PULSE_LEN_MIN && irmp_pulse_time <= BANG_OLUFSEN_PULSE_LEN_MAX)
                        {
                            if (irmp_bit == 1)                                      // Bang & Olufsen: 3rd bit
                            {
                                if (irmp_pause_time >= BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MAX)
                                {
                                    DEBUG_PRINTF ("3rd start bit\n");
                                    wait_for_space = 0;
                                    irmp_tmp_id = 0;
                                    irmp_bit++;
                                }
                                else
                                {                                                       // timing incorrect!
                                    DEBUG_PRINTF ("error 3a B&O: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                                    irmp_start_bit_detected = 0;                    // reset flags and wait for next start bit
                                    irmp_pause_time         = 0;
                                }
                            }
                            else if (irmp_bit == 19)                                // Bang & Olufsen: trailer bit
                            {
                                if (irmp_pause_time >= BANG_OLUFSEN_TRAILER_BIT_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_TRAILER_BIT_PAUSE_LEN_MAX)
                                {
                                    DEBUG_PRINTF ("trailer bit\n");
                                    wait_for_space = 0;
                                    irmp_tmp_id = 0;
                                    irmp_bit++;
                                }
                                else
                                {                                                   // timing incorrect!
                                    DEBUG_PRINTF ("error 3b B&O: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                                    irmp_start_bit_detected = 0;                    // reset flags and wait for next start bit
                                    irmp_pause_time         = 0;
                                }
                            }
                            else
                            {
                                if (irmp_pause_time >= BANG_OLUFSEN_1_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_1_PAUSE_LEN_MAX)
                                {                                                   // pulse & pause timings correct for "1"?
                                    DEBUG_PUTCHAR ('1');
                                    DEBUG_PUTCHAR ('\n');
                                    irmp_store_bit (1);
                                    last_value = 1;
                                    wait_for_space = 0;
                                }
                                else if (irmp_pause_time >= BANG_OLUFSEN_0_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_0_PAUSE_LEN_MAX)
                                {                                                   // pulse & pause timings correct for "0"?
                                    DEBUG_PUTCHAR ('0');
                                    DEBUG_PUTCHAR ('\n');
                                    irmp_store_bit (0);
                                    last_value = 0;
                                    wait_for_space = 0;
                                }
                                else if (irmp_pause_time >= BANG_OLUFSEN_R_PAUSE_LEN_MIN && irmp_pause_time <= BANG_OLUFSEN_R_PAUSE_LEN_MAX)
                                {
                                    DEBUG_PUTCHAR (last_value + '0');
                                    DEBUG_PUTCHAR ('\n');
                                    irmp_store_bit (last_value);
                                    wait_for_space = 0;
                                }
                                else
                                {                                                   // timing incorrect!
                                    DEBUG_PRINTF ("error 3c B&O: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                                    irmp_start_bit_detected = 0;                    // reset flags and wait for next start bit
                                    irmp_pause_time         = 0;
                                }
                            }
                        }
                        else
                        {                                                           // timing incorrect!
                            DEBUG_PRINTF ("error 3d B&O: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                            irmp_start_bit_detected = 0;                            // reset flags and wait for next start bit
                            irmp_pause_time         = 0;
                        }
                    }
                    else
#endif // IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL

                    if (irmp_pulse_time >= irmp_param.pulse_1_len_min && irmp_pulse_time <= irmp_param.pulse_1_len_max &&
                        irmp_pause_time >= irmp_param.pause_1_len_min && irmp_pause_time <= irmp_param.pause_1_len_max)
                    {                                                               // pulse & pause timings correct for "1"?
                        DEBUG_PUTCHAR ('1');
                        DEBUG_PUTCHAR ('\n');
                        irmp_store_bit (1);
                        wait_for_space = 0;
                    }
                    else if (irmp_pulse_time >= irmp_param.pulse_0_len_min && irmp_pulse_time <= irmp_param.pulse_0_len_max &&
                             irmp_pause_time >= irmp_param.pause_0_len_min && irmp_pause_time <= irmp_param.pause_0_len_max)
                    {                                                               // pulse & pause timings correct for "0"?
                        DEBUG_PUTCHAR ('0');
                        DEBUG_PUTCHAR ('\n');
                        irmp_store_bit (0);
                        wait_for_space = 0;
                    }
                    else
                    {                                                               // timing incorrect!
                        DEBUG_PRINTF ("error 3: timing not correct: data bit %d,  pulse: %d, pause: %d\n", irmp_bit, irmp_pulse_time, irmp_pause_time);
                        irmp_start_bit_detected = 0;                                // reset flags and wait for next start bit
                        irmp_pause_time         = 0;
                    }

                    irmp_pulse_time = 1;                                            // set counter to 1, not 0
                }
            }
            else
            {                                                                       // counting the pulse length ...
                if (!irmp_input)                                                    // still light?
                {                                                                   // yes...
                    irmp_pulse_time++;                                              // increment counter
                }
                else
                {                                                                   // now it's dark!
                    wait_for_space  = 1;                                            // let's count the time (see above)
                    irmp_pause_time = 1;                                            // set pause counter to 1, not 0
                }
            }

            if (irmp_bit == irmp_param.complete_len && irmp_param.stop_bit == 0)    // enough bits received?
            {
                if (last_irmp_command == irmp_tmp_command && repetition_counter < AUTO_REPETITION_LEN)
                {
                    repetition_frame_number++;
                }
                else
                {
                    repetition_frame_number = 0;
                }

#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1
                // if SIRCS protocol and the code will be repeated within 50 ms, we will ignore 2nd and 3rd repetition frame
                if (irmp_param.protocol == IRMP_SIRCS_PROTOCOL && (repetition_frame_number == 1 || repetition_frame_number == 2))
                {
                    DEBUG_PRINTF ("code skipped: SIRCS auto repetition frame #%d, counter = %d, auto repetition len = %d\n",
                                    repetition_frame_number + 1, repetition_counter, AUTO_REPETITION_LEN);
                    repetition_counter = 0;
                }
                else
#endif

#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
                // if SAMSUNG32 protocol and the code will be repeated within 50 ms, we will ignore every 2nd frame
                if (irmp_param.protocol == IRMP_SAMSUNG32_PROTOCOL && (repetition_frame_number & 0x01))
                {
                    DEBUG_PRINTF ("code skipped: SAMSUNG32 auto repetition frame #%d, counter = %d, auto repetition len = %d\n",
                                    repetition_frame_number + 1, repetition_counter, AUTO_REPETITION_LEN);
                    repetition_counter = 0;
                }
                else
#endif

#if IRMP_SUPPORT_NUBERT_PROTOCOL == 1
                // if NUBERT protocol and the code will be repeated within 50 ms, we will ignore it.
                if (irmp_param.protocol == IRMP_NUBERT_PROTOCOL && (repetition_frame_number & 0x01))
                {
                    DEBUG_PRINTF ("code skipped: NUBERT auto repetition frame #%d, counter = %d, auto repetition len = %d\n",
                                    repetition_frame_number + 1, repetition_counter, AUTO_REPETITION_LEN);
                    repetition_counter = 0;
                }
                else
#endif

                {
                    DEBUG_PRINTF ("code detected, length = %d\n", irmp_bit);
                    irmp_ir_detected = TRUE;

#if IRMP_SUPPORT_DENON_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_DENON_PROTOCOL)
                    {                                                               // check for repetition frame
                        if ((~irmp_tmp_command & 0x3FF) == last_irmp_denon_command) // command bits must be inverted
                        {
                            irmp_tmp_command = last_irmp_denon_command;             // use command received before!

                            irmp_protocol = irmp_param.protocol;                    // store protocol
                            irmp_address = irmp_tmp_address;                        // store address
                            irmp_command = irmp_tmp_command ;                       // store command
                        }
                        else
                        {
                            DEBUG_PRINTF ("waiting for inverted command repetition\n");
                            irmp_ir_detected = FALSE;
                            last_irmp_denon_command = irmp_tmp_command;
                        }
                    }
                    else
#endif // IRMP_SUPPORT_DENON_PROTOCOL

#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
                    if (irmp_param.protocol == IRMP_GRUNDIG_PROTOCOL && irmp_tmp_command == 0x01ff)     // only start frame?
                    {
                        DEBUG_PRINTF ("Detected start frame, ignoring it\n");
                        irmp_ir_detected = FALSE;
                        // last_irmp_grundig_command = irmp_tmp_command;
                    }
                    else
#endif // IRMP_SUPPORT_DENON_PROTOCOL
                    {
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
                        if (irmp_param.protocol == IRMP_NEC_PROTOCOL && irmp_bit == 0)  // repetition frame
                        {
                            irmp_tmp_address = last_irmp_address;                   // address is last address
                            irmp_tmp_command = last_irmp_command;                   // command is last command
                            irmp_flags |= IRMP_FLAG_REPETITION;
                        }
#endif // IRMP_SUPPORT_NEC_PROTOCOL
                        irmp_protocol = irmp_param.protocol;
                        irmp_address = irmp_tmp_address;                            // store address
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
                        last_irmp_address = irmp_tmp_address;                       // store as last address, too
#endif

#if IRMP_SUPPORT_RC5_PROTOCOL == 1
                        irmp_tmp_command |= rc5_cmd_bit6;                           // store bit 6
#endif
                        irmp_command = irmp_tmp_command;                            // store command

#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
                        irmp_id = irmp_tmp_id;
#endif
                    }
                }

                if (irmp_ir_detected)
                {
                    if (last_irmp_command == irmp_command &&
                        last_irmp_address == irmp_address &&
                        repetition_counter < IRMP_REPETITION_TIME)
                    {
                        irmp_flags |= IRMP_FLAG_REPETITION;
                    }

                    last_irmp_address = irmp_tmp_address;                           // store as last address, too
                    last_irmp_command = irmp_tmp_command;                           // store as last command, too

                    repetition_counter = 0;
                }

                irmp_start_bit_detected = 0;                                        // and wait for next start bit
                irmp_tmp_command        = 0;
                irmp_pulse_time         = 0;
                irmp_pause_time         = 0;
            }
        }
    }
}

#ifdef DEBUG

// main function - for unix/linux + windows only!
// AVR: see main.c!
// Compile it under linux with:
// cc irmp.c -o irmp
//
// usage: ./irmp [-v|-s|-a] < file

static void
print_timings (void)
{
    printf ("PROTOCOL       START BIT NO.   START BIT PULSE     START BIT PAUSE\n");
    printf ("====================================================================================\n");
    printf ("SIRCS          1               %3d - %3d           %3d - %3d\n",
            SIRCS_START_BIT_PULSE_LEN_MIN, SIRCS_START_BIT_PULSE_LEN_MAX, SIRCS_START_BIT_PAUSE_LEN_MIN, SIRCS_START_BIT_PAUSE_LEN_MAX);
    printf ("NEC            1               %3d - %3d           %3d - %3d\n",
            NEC_START_BIT_PULSE_LEN_MIN, NEC_START_BIT_PULSE_LEN_MAX, NEC_START_BIT_PAUSE_LEN_MIN, NEC_START_BIT_PAUSE_LEN_MAX);
    printf ("NEC (rep)      1               %3d - %3d           %3d - %3d\n",
            NEC_START_BIT_PULSE_LEN_MIN, NEC_START_BIT_PULSE_LEN_MAX, NEC_REPEAT_START_BIT_PAUSE_LEN_MIN, NEC_REPEAT_START_BIT_PAUSE_LEN_MAX);
    printf ("SAMSUNG        1               %3d - %3d           %3d - %3d\n",
            SAMSUNG_START_BIT_PULSE_LEN_MIN, SAMSUNG_START_BIT_PULSE_LEN_MAX, SAMSUNG_START_BIT_PAUSE_LEN_MIN, SAMSUNG_START_BIT_PAUSE_LEN_MAX);
    printf ("MATSUSHITA     1               %3d - %3d           %3d - %3d\n",
            MATSUSHITA_START_BIT_PULSE_LEN_MIN, MATSUSHITA_START_BIT_PULSE_LEN_MAX, MATSUSHITA_START_BIT_PAUSE_LEN_MIN, MATSUSHITA_START_BIT_PAUSE_LEN_MAX);
    printf ("KASEIKYO       1               %3d - %3d           %3d - %3d\n",
            KASEIKYO_START_BIT_PULSE_LEN_MIN, KASEIKYO_START_BIT_PULSE_LEN_MAX, KASEIKYO_START_BIT_PAUSE_LEN_MIN, KASEIKYO_START_BIT_PAUSE_LEN_MAX);
    printf ("RECS80         1               %3d - %3d           %3d - %3d\n",
            RECS80_START_BIT_PULSE_LEN_MIN, RECS80_START_BIT_PULSE_LEN_MAX, RECS80_START_BIT_PAUSE_LEN_MIN, RECS80_START_BIT_PAUSE_LEN_MAX);
    printf ("RC5            1               %3d - %3d           %3d - %3d\n",
            RC5_START_BIT_LEN_MIN, RC5_START_BIT_LEN_MAX, RC5_START_BIT_LEN_MIN, RC5_START_BIT_LEN_MAX);
    printf ("DENON          1               %3d - %3d           %3d - %3d or %3d - %3d\n",
            DENON_PULSE_LEN_MIN, DENON_PULSE_LEN_MAX, DENON_1_PAUSE_LEN_MIN, DENON_1_PAUSE_LEN_MAX, DENON_0_PAUSE_LEN_MIN, DENON_0_PAUSE_LEN_MAX);
    printf ("RC6            1               %3d - %3d           %3d - %3d\n",
            RC6_START_BIT_PULSE_LEN_MIN, RC6_START_BIT_PULSE_LEN_MAX, RC6_START_BIT_PAUSE_LEN_MIN, RC6_START_BIT_PAUSE_LEN_MAX);
    printf ("RECS80EXT      1               %3d - %3d           %3d - %3d\n",
            RECS80EXT_START_BIT_PULSE_LEN_MIN, RECS80EXT_START_BIT_PULSE_LEN_MAX, RECS80EXT_START_BIT_PAUSE_LEN_MIN, RECS80EXT_START_BIT_PAUSE_LEN_MAX);
    printf ("NUBERT         1               %3d - %3d           %3d - %3d\n",
            NUBERT_START_BIT_PULSE_LEN_MIN, NUBERT_START_BIT_PULSE_LEN_MAX, NUBERT_START_BIT_PAUSE_LEN_MIN, NUBERT_START_BIT_PAUSE_LEN_MAX);
    printf ("BANG_OLUFSEN   1               %3d - %3d           %3d - %3d\n",
            BANG_OLUFSEN_START_BIT1_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT1_PULSE_LEN_MAX, BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT1_PAUSE_LEN_MAX);
    printf ("BANG_OLUFSEN   2               %3d - %3d           %3d - %3d\n",
            BANG_OLUFSEN_START_BIT2_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT2_PULSE_LEN_MAX, BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT2_PAUSE_LEN_MAX);
    printf ("BANG_OLUFSEN   3               %3d - %3d           %3d - %3d\n",
            BANG_OLUFSEN_START_BIT3_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT3_PULSE_LEN_MAX, BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT3_PAUSE_LEN_MAX);
    printf ("BANG_OLUFSEN   4               %3d - %3d           %3d - %3d\n",
            BANG_OLUFSEN_START_BIT4_PULSE_LEN_MIN, BANG_OLUFSEN_START_BIT4_PULSE_LEN_MAX, BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MIN, BANG_OLUFSEN_START_BIT4_PAUSE_LEN_MAX);
    printf ("GRUNDIG        1               %3d - %3d           %3d - %3d\n",
            GRUNDIG_START_BIT_LEN_MIN, GRUNDIG_START_BIT_LEN_MAX, GRUNDIG_PRE_PAUSE_LEN_MIN, GRUNDIG_PRE_PAUSE_LEN_MAX);
}

int
main (int argc, char ** argv)
{
    int         i;
    int         verbose = FALSE;
    int         analyze = FALSE;
    int         ch;
    int         last_ch = 0;
    int         pulse = 0;
    int         pause = 0;

    int         min_pulse_long = 100000;
    int         max_pulse_long = 0;
    int         sum_pulses_long = 0;
    int         n_pulses_long = 0;

    int         min_pulse_short = 100000;
    int         max_pulse_short = 0;
    int         sum_pulses_short = 0;
    int         n_pulses_short = 0;

    int         min_pause_long = 100000;
    int         max_pause_long = 0;
    int         sum_pauses_long = 0;
    int         n_pauses_long = 0;

    int         min_pause_short = 100000;
    int         max_pause_short = 0;
    int         sum_pauses_short = 0;
    int         n_pauses_short = 0;

    int         min_start_pulse = 100000;
    int         max_start_pulse = 0;
    int         sum_start_pulses = 0;
    int         n_start_pulses = 0;

    int         min_start_pause = 100000;
    int         max_start_pause = 0;
    int         sum_start_pauses = 0;
    int         n_start_pauses = 0;

    int         first_pulse = TRUE;
    int         first_pause = TRUE;

    IRMP_DATA   irmp_data;

    if (argc == 2)
    {
        if (! strcmp (argv[1], "-v"))
        {
            verbose = TRUE;
        }
        else if (! strcmp (argv[1], "-a"))
        {
            analyze = TRUE;
            verbose = TRUE;
        }
        else if (! strcmp (argv[1], "-s"))
        {
            silent = TRUE;
        }
        else if (! strcmp (argv[1], "-p"))
        {
            print_timings ();
            return (0);
        }
    }

    IRMP_PIN = 0xFF;

    while ((ch = getchar ()) != EOF)
    {
        if (ch == '_' || ch == '0')
        {
            if (last_ch != ch)
            {
                if (verbose && pause > 0)
                {
                    printf ("pause: %d\n", pause);

                    if (first_pause)
                    {
                        if (min_start_pause > pause)
                        {
                            min_start_pause = pause;
                        }
                        if (max_start_pause < pause)
                        {
                            max_start_pause = pause;
                        }
                        n_start_pauses++;
                        sum_start_pauses += pause;
                        first_pause = FALSE;
                    }
                    else
                    {
                        if (pause >= 10)
                        {
                            if (pause > 100)                                        // perhaps repetition frame follows
                            {
                                first_pulse = TRUE;
                                first_pause = TRUE;
                            }
                            else
                            {
                                if (min_pause_long > pause)
                                {
                                    min_pause_long = pause;
                                }
                                if (max_pause_long < pause)
                                {
                                    max_pause_long = pause;
                                }
                                n_pauses_long++;
                                sum_pauses_long += pause;
                            }
                        }
                        else
                        {
                            if (min_pause_short > pause)
                            {
                                min_pause_short = pause;
                            }
                            if (max_pause_short < pause)
                            {
                                max_pause_short = pause;
                            }
                            n_pauses_short++;
                            sum_pauses_short += pause;
                        }
                    }
                }
                pause = 0;
            }
            pulse++;
            IRMP_PIN = 0x00;
        }
        else if (ch == 0xaf || ch == '-' || ch == '1')
        {
            if (last_ch != ch)
            {
                if (verbose)
                {
                    printf ("pulse: %d ", pulse);

                    if (first_pulse)
                    {
                        if (min_start_pulse > pulse)
                        {
                            min_start_pulse = pulse;
                        }
                        if (max_start_pulse < pulse)
                        {
                            max_start_pulse = pulse;
                        }
                        n_start_pulses++;
                        sum_start_pulses += pulse;
                        first_pulse = FALSE;
                    }
                    else
                    {
                        if (pulse >= 10)
                        {
                            if (min_pulse_long > pulse)
                            {
                                min_pulse_long = pulse;
                            }
                            if (max_pulse_long < pulse)
                            {
                                max_pulse_long = pulse;
                            }
                            n_pulses_long++;
                            sum_pulses_long += pulse;
                        }
                        else
                        {
                            if (min_pulse_short > pulse)
                            {
                                min_pulse_short = pulse;
                            }
                            if (max_pulse_short < pulse)
                            {
                                max_pulse_short = pulse;
                            }
                            n_pulses_short++;
                            sum_pulses_short += pulse;
                        }
                    }
                }
                pulse = 0;
            }
            pause++;
            IRMP_PIN = 0xff;
        }
        else if (ch == '\n')
        {
            IRMP_PIN = 0xff;

            if (verbose && pause > 0)
            {
                printf ("pause: %d\n", pause);
            }
            pause = 0;

            if (! analyze)
            {
                for (i = 0; i < 8000; i++)                                              // newline: long pause of 800 msec
                {
                    irmp_ISR ();
                }
            }
            first_pulse = TRUE;
            first_pause = TRUE;
        }
        else if (ch == '#')
        {
            puts ("-------------------------------------------------------------------");
            putchar (ch);

            while ((ch = getchar()) != '\n' && ch != EOF)
            {
                if (ch != '\r')                                                     // ignore CR in DOS/Windows files
                {
                    putchar (ch);
                }
            }
            putchar ('\n');
        }

        last_ch = ch;

        if (! analyze)
        {
            irmp_ISR ();
        }

        if (irmp_get_data (&irmp_data))
        {
            printf ("protcol = %d, address = 0x%04x, code = 0x%04x, flags = 0x%02x\n",
                    irmp_data.protocol, irmp_data.address, irmp_data.command, irmp_data.flags);
        }
    }

    if (analyze)
    {
        printf ("\nSTATITSTICS:\n");
        printf ("---------------------------------\n");
        printf ("number of start pulses:     %d\n", n_start_pulses);
        printf ("minimum start pulse length: %d usec\n", (1000000 * min_start_pulse) / F_INTERRUPTS);
        printf ("maximum start pulse length: %d usec\n", (1000000 * max_start_pulse) / F_INTERRUPTS);
        if (n_start_pulses > 0)
        {
            printf ("average start pulse length: %d usec\n", ((1000000 * sum_start_pulses) / n_start_pulses) / F_INTERRUPTS);
        }
        putchar ('\n');
        printf ("number of start pauses:     %d\n", n_start_pauses);
        if (n_start_pauses > 0)
        {
            printf ("minimum start pause length: %d usec\n", (1000000 * min_start_pause) / F_INTERRUPTS);
            printf ("maximum start pause length: %d usec\n", (1000000 * max_start_pause) / F_INTERRUPTS);
            printf ("average start pause length: %d usec\n", ((1000000 * sum_start_pauses) / n_start_pauses) / F_INTERRUPTS);
        }
        putchar ('\n');
        printf ("number of long pulses:     %d\n", n_pulses_long);
        if (n_pulses_long > 0)
        {
            printf ("minimum long pulse length: %d usec\n", (1000000 * min_pulse_long) / F_INTERRUPTS);
            printf ("maximum long pulse length: %d usec\n", (1000000 * max_pulse_long) / F_INTERRUPTS);
            printf ("average long pulse length: %d usec\n", ((1000000 * sum_pulses_long) / n_pulses_long) / F_INTERRUPTS);
        }
        putchar ('\n');
        printf ("number of short pulses:     %d\n", n_pulses_short);
        if (n_pulses_short > 0)
        {
            printf ("minimum short pulse length: %d usec\n", (1000000 * min_pulse_short) / F_INTERRUPTS);
            printf ("maximum short pulse length: %d usec\n", (1000000 * max_pulse_short) / F_INTERRUPTS);
            printf ("average short pulse length: %d usec\n", ((1000000 * sum_pulses_short) / n_pulses_short) / F_INTERRUPTS);

        }
        putchar ('\n');
        printf ("number of long pauses:     %d\n", n_pauses_long);
        if (n_pauses_long > 0)
        {
            printf ("minimum long pause length: %d usec\n", (1000000 * min_pause_long) / F_INTERRUPTS);
            printf ("maximum long pause length: %d usec\n", (1000000 * max_pause_long) / F_INTERRUPTS);
            printf ("average long pause length: %d usec\n", ((1000000 * sum_pauses_long) / n_pauses_long) / F_INTERRUPTS);
        }
        putchar ('\n');
        printf ("number of short pauses:     %d\n", n_pauses_short);
        if (n_pauses_short > 0)
        {
            printf ("minimum short pause length: %d usec\n", (1000000 * min_pause_short) / F_INTERRUPTS);
            printf ("maximum short pause length: %d usec\n", (1000000 * max_pause_short) / F_INTERRUPTS);
            printf ("average short pause length: %d usec\n", ((1000000 * sum_pauses_short) / n_pauses_short) / F_INTERRUPTS);
        }
    }
    return 0;
}

#endif // DEBUG
