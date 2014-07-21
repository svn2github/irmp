/*
                                                    W. Strobl, Bonn, March 2014
                                        
Simple minded software bit banging async RS232 implementation for Microchip XC8

http://en.wikipedia.org/wiki/Bit_banging

Tested with 2400, 9600 and 19200 baud on a 4 MHz 16F675 so far
Tested with 9600 baud on a 32 MHz 12F1840. 
19200 softuard doesn't work.

                              PIC12F1840

       ___                     __
 10k -|___|-+           Vdd -o| o|o- Vss
       ___  +-RS232in / GP5 -o|  |o- GP0 / ICSPDAT
  1k -|___|-- RS232out/ GP4 -o|  |o- GP1 / ICSPCLK
                  Vpp / GP3 -o|__|o- GP2 /

Necessary definitions (examples)

#define FCY 1000000             // cycles per second (4 MHz PIC)
#define SOFTUART_RXPIN  GPIO5   // if input is desired
#define SOFTUART_TXPIN  GPIO4   // if output is desired

Optional definitions

#define SOFTUART_BAUD 19200     // default: 9600
#define SOFTUART_STDIO 1        // if definition for getch, putch is desired
                                // default: 0
#define SOFTUART_DI 1           // if interrupts are to be disabled during IO
                                // default: 0
#define SOFTUART_MARK 0         // 0: not inverted (default: 1)


Typical:


#define SOFTUART_RXPIN  GPIO5
#define SOFTUART_TXPIN  GPIO4
#define SOFTUART_STDIO 1
#define SOFTUART_DI 1
#include "softuart.h"

#define kbhit softuartkbhit

*/

/******************************************************************************/
// Software UART 
/******************************************************************************/
// FCY == instructions per second, see system.h for xc8
#ifndef SOFTUART_BAUD           // default baudrate
#define SOFTUART_BAUD 9600
#endif
#define SOFTUART_BITLEN (FCY/SOFTUART_BAUD)
#define SOFTUART_DELAY (SOFTUART_BITLEN/5)

#ifndef SOFTUART_MARK
#define SOFTUART_MARK 1         // 0: not inverted (default: 1)
#endif


// Input Pin defined?
#ifdef SOFTUART_RXPIN
/******************************************************************************/
// Input
/******************************************************************************/

char softuartgetch(void)
{
    char rcvd,i; 
#if SOFTUART_DI
    di();
#endif
    rcvd=0;
    _delay(SOFTUART_BITLEN/2-10); // wait half a startbit
    if (SOFTUART_RXPIN != SOFTUART_MARK) 
    {
#if SOFTUART_DI
        ei();
#endif
        return 0; // glitch
    }
    _delay(SOFTUART_BITLEN/2-10); // wait half a startbit
    for (i=0;i<8;i++)
    {
        rcvd >>= 1; // shift previous bits, LSB comes first
        _delay(SOFTUART_BITLEN/2-12); // ADJUST
        rcvd |= ((SOFTUART_RXPIN != SOFTUART_MARK)?0x80:0);
        _delay(SOFTUART_BITLEN/2-10); // ADJUST
    }
#ifdef SOFTUART_DI
    ei();
#endif
    _delay(SOFTUART_BITLEN); // stopbit
    return rcvd;
}

#define softuartkbhit() (SOFTUART_RXPIN == SOFTUART_MARK)

#endif

// Output Pin defined?
#ifdef SOFTUART_TXPIN
/******************************************************************************/
// Output 
//******************************************************************************/

#if defined(SOFTUART_TXPIN) || defined(SOFTUART_STDIO)
#define softuartputch putch
#endif

void softuartputch(char c)
{
    char i;
#ifdef SOFTUART_DI
        di();
#endif
    SOFTUART_TXPIN = SOFTUART_MARK; // startbit 
    _delay(SOFTUART_BITLEN-20); // Adjust Schleifeninit braucht 
    for (i=0;i<8;i++)
    {
        SOFTUART_TXPIN = c; // checken, ob da das untere Bit verwendet wird sonst
#if SOFTUART_MARK
        SOFTUART_TXPIN = ~(c&1);
#else
        SOFTUART_TXPIN = c&1;
#endif
        c >>=1;
        _delay(SOFTUART_BITLEN-20); // Adjust
    }
    SOFTUART_TXPIN = ~SOFTUART_MARK;
    _delay(SOFTUART_BITLEN*2); // two stop bits
#ifdef SOFTUART_DI
    ei();
#endif
}
#endif

/******************************************************************************/
// Utility
/******************************************************************************/
#if defined(SOFTUART_RXPIN) || defined(SOFTUART_STDIO)
// getch with wait
char getch(void)
{
    while (!softuartkbhit()) _delay(1);
    return softuartgetch();
}
#endif
