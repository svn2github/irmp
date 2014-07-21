/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main_pic.c - example main module 
 * 
 * IR decoder using IRMP
 *
 * (c) 2014 Wolfgang Strobl (ws at mystrobl.de) 2014-03-12:2014-07-05
 *
 * This demo module is runnable on a Microchip PIC 12F1840
 *
 * To be used with IRMP by Frank Meyer (frank(at)fli4l.de)
 * <http://www.mikrocontroller.net/articles/IRMP>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
 
 
/* 
                              PIC12F1840
       ___                     __
 10k -|___|-+           Vdd -o| o|o- Vss
       ___  +-RS232in / GP5 -o|  |o- GP0 / ICSPDAT
  1k -|___|-- RS232out/ GP4 -o|  |o- GP1 / ICSPCLK 
                  Vpp / GP3 -o|__|o- GP2 / TS TSOP1736

Example output, using a bunch of different remote controls

IRMP PIC 12F1840 1.1 ws
P 7 a=0x0011 c=0x000c f=0x00 (RC5)
P 6 a=0x0001 c=0x0018 f=0x00 (RECS80)
P 2 a=0xbf00 c=0x0019 f=0x00 (NEC)
P 2 a=0xeb14 c=0x0001 f=0x00 (NEC)
P 7 a=0x001c c=0x0005 f=0x00 (RC5)
P 7 a=0x000a c=0x0057 f=0x00 (RC5)
P 7 a=0x000a c=0x0057 f=0x01 (RC5)
P 2 a=0xfb04 c=0x0008 f=0x00 (NEC)

*/

#include <stdio.h>

#include "irmp.h"

/******************************************************************************/
// "system.h"
/******************************************************************************/
#define SYS_FREQ        32000000L
#define _XTAL_FREQ      32000000   // for _delay
#define FCY             (SYS_FREQ/4)

/******************************************************************************/
// "user.c"
/******************************************************************************/
void InitApp(void)
{
    ANSELA=0;
    TRISA4=0;
    IRCF0=0; // p. 45
    IRCF1=1;
    IRCF2=1;
    IRCF3=1;
    SPLLEN=1; // p  46 and 54 
}

/******************************************************************************/
// "configuration_bits.c" 12F1848
/******************************************************************************/
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)
/******************************************************************************/

/******************************************************************************/
// UART
/******************************************************************************/

// This demo module uses RS232 TX via EUSART, only

#define useEUSART 1
#define BAUD 19200


void 
RS232init(void)
 {
     // Transmit
     TXCKSEL = 1;                 //  put TX on pin 4 - not 0 -, p 102
     SPBRGL = (_XTAL_FREQ/BAUD/64-1);  
     SPBRGH = 0;
     BRGH = 0;
     BRG16 = 0;
     // p 259 manual
     SYNC = 0;                  // 0 p. 267
     SPEN = 1;                  //  26.1.1.7
     SCKP = 1;                  // invert p 269
     TXEN = 1;
 }

 // EUSART transmit
 void 
 putch(char c)
 {
    while (!TRMT) _delay(1);
    TXREG=c;
 }
 
/******************************************************************************/
// Timer and ISR
/******************************************************************************/
 
void 
timer1_init(void)
{
    // p 154
    TMR1=0xFC00; // p. 155 wait 1024 cycles for stabilization.
    TMR1CS1=0; // Clock source == System Clock
    TMR1CS0=1; 
    TMR1IE=1; // enable TMR1 interrupts
    PEIE=1;   // enable Pheripheral Interrupts
    
}


/******************************************************************************/
// Interrupt handler
/******************************************************************************/

void interrupt isr(void)
{
    irmp_ISR();
    TMR1=0xffff-_XTAL_FREQ/F_INTERRUPTS; 
    TMR1IF=0; // clear timer 1 interrupt
}

/******************************************************************************/
// MAIN
/******************************************************************************/

int
main (void)
{
    IRMP_DATA irmp_data;
    InitApp(); // später inlinen
    
#if useEUSART
    RS232init();
#endif
    __delay_ms(200);
    printf("IRMP PIC 12F1840 1.1 ws\r\n");
    irmp_init();                                                            // initialize irmp
    timer1_init();            // initialize timer1
    ei();                     // enable interrupts
    TMR1ON=1; // start timer
  
    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {
            printf("P ");
            printf("%d a=0x%04x c=0x%04x f=0x%02x (",irmp_data.protocol, irmp_data.address,irmp_data.command,irmp_data.flags); 
            
            
#if IRMP_PROTOCOL_NAMES
            printf(irmp_protocol_names[irmp_data.protocol]);
#else            
            switch(irmp_data.protocol)
            {
                case 1:
                    printf("Sony");
                    break;
                case 2:
                    printf("NEC");
                    break;
                case 7:
                    printf("RC5");
                    break;
                case 0x21:
                    printf("Ortek");
                    break;
            }
#endif
            printf(")\r\n");
        }
    }
}
