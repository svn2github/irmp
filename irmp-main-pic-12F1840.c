/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main_pic12f1840.c - example main module for PIC 12f1840
 * 
 * IR decoder using IRMP
 *
 * (c) 2014 Wolfgang Strobl (news4 at mystrobl.de) 2014-03-12:2014-07-20
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

Hauptprogramm fuer den nachfolgenden Testaufbau, bestehend aus zwei mal 


             TSOP1736+        PIC12F1840
                     |         __
              1k     |  Vdd--o| o|o--Vss
             ___     +--GP5--o|  |o--GP0 / ICSPDAT
RS232 out  -|___|-------GP4--o|  |o--GP1 / ICSPCLK  ___    LED
RS232 in   -|___|---Vpp/GP3--o|__|o--GP2-----------|___|- ->|---Vss
             10k
             
auf einem Steckbrett. (Genauer gesagt, dies ist die aktuelle Beschaltung
fuer V1.8,  V1.0 ist aber bzgl. IRMP-Empfang funktional identisch. Nicht eingezeichnet
ist ein Abblockkondensator von 100nF ueber Vss und Vdd.

Uebersetzt mit Microchip MPLAB XC8 C Compiler (Free Mode) V1.31 
im stark gecrippelten "Free Mode".

Memory Summary: (V 1.8)
    Program space        used   C6Fh (  3183) of  1000h words   ( 77.7%)
    Data space           used    9Bh (   155) of   100h bytes   ( 60.5%)
    EEPROM space         used     0h (     0) of   100h bytes   (  0.0%)
    Data stack space     used     0h (     0) of    5Eh bytes   (  0.0%)
    Configuration bits   used     2h (     2) of     2h words   (100.0%)
    ID Location space    used     0h (     0) of     4h bytes   (  0.0%)


Testaufbau: 

Zwei Steckbretter,
urspruengliche Version des Programms als Empfaenger, 
aktuelle Version als Sender, Aufzeichnung mit putty,
angeschlossen jeweils per USB2RS232-Kabel von Conrad
(972543, basierend auf Prolific PL2303). Soft-UART 
fuer Input, da 12F1820 keine Kontrolle ueber Input-
Polaritaet erlaubt und ich fuer Testaufbauten eine
Minimalbeschaltung bevorzuge.

Kurze Distanz
(~30 cm) zwischen Sender und Empfaenger), keine genaue Ausrichtung.
Stromversorgung wahlwweise mit 5V via PICkit 2 oder 3x1.2V NiMH-AA.

Zunaechst 
CD TAPE TUNER AUX OFF mit Philips FB, 
OFF mit VAOVA TV-2900HDD FB
dann Eingabe . und n beim Sender.

Sender:

IRMP PIC 12F1840 1.8 ws
P 7 a=0x0014 c=0x003f f=0x00 (RC5)
P 7 a=0x0014 c=0x003f f=0x01 (RC5)
P 7 a=0x0012 c=0x003f f=0x00 (RC5)
P 7 a=0x0011 c=0x003f f=0x00 (RC5)
P 7 a=0x0015 c=0x003f f=0x00 (RC5)
P 7 a=0x0015 c=0x000c f=0x00 (RC5)
P 2 a=0xbf00 c=0x0059 f=0x00 (NEC)
P 2 a=0xbf00 c=0x0059 f=0x01 (NEC)
. MX115OFF PR2 221
n NEC PR2 209

Empfaenger:

IRMP PIC 12F1840 1.0 ws
P 7 a=0x0014 c=0x003f f=0x00 (RC5)
P 7 a=0x0014 c=0x003f f=0x01 (RC5)
P 7 a=0x0012 c=0x003f f=0x00 (RC5)
P 7 a=0x0011 c=0x003f f=0x00 (RC5)
P 7 a=0x0015 c=0x000c f=0x00 (RC5)
P 2 a=0xbf00 c=0x0059 f=0x00 (NEC)
P 2 a=0xbf00 c=0x0059 f=0x01 (NEC)
P 7 a=0x0015 c=0x000c f=0x00 (RC5)
P 7 a=0x0015 c=0x000c f=0x01 (RC5)
P 7 a=0x0015 c=0x000c f=0x01 (RC5)
P 2 a=0x0055 c=0x00aa f=0x00 (NEC)

Die via DSO an der LED gemessenen Frequenzen sind 36.0 resp. 38.0 kHz

*/

#include <stdio.h>

#include "irmp.h"
#include "irsnd.h"

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

#define GPIO3 RA3 
#define GPIO4 RA4 

#define SOFTUART_RXPIN  GPIO3
#define SOFTUART_STDIO 1
#define SOFTUART_DI 1


#define BAUD 19200 // 38200 ginge auch noch
#define SOFTUART_BAUD BAUD
#include "softuart_pic.h"

#define kbhit softuartkbhit


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
    TMR1IF=0;
    TMR1ON=1;
}


/******************************************************************************/
// Interrupt handler
/******************************************************************************/

void interrupt isr(void)
{
    TMR1=0xffff-_XTAL_FREQ/F_INTERRUPTS; 
    TMR1IF=0; // clear timer 1 interrupt
 
    if (!irsnd_ISR())
    {
        irmp_ISR();
    }
}


IRMP_DATA irmp_data;

void RC5(uint16_t addr,uint16_t  cmd, uint8_t repetitions)
{
    irmp_data.protocol = IRMP_RC5_PROTOCOL;                       
    irmp_data.address  = addr;
    irmp_data.command  = cmd;
    irmp_data.flags    = repetitions;
    irsnd_send_data (&irmp_data, FALSE); 
}

void NEC(int addr,int cmd)
{
    irmp_data.protocol = IRMP_NEC_PROTOCOL;                       
    irmp_data.address  = addr;
    irmp_data.command  = cmd;
    irmp_data.flags    = 0;
    irsnd_send_data (&irmp_data, FALSE); 
}


/******************************************************************************/
// MAIN
/******************************************************************************/

int
main (void)
{
    IRMP_DATA irmp_data;
    char c;
    InitApp(); 

    PWMoff();
    RS232init();
    
    __delay_ms(200);
    printf("IRMP PIC 12F1840 1.8 ws\r\n");
    irmp_init();              // initialize irmp
    timer1_init();            // initialize timer1
    ei();                     // enable interrupts
    TMR1ON=1; // start timer
  
    for (;;)
    {
        if (kbhit())
        {
            c=getch();
            if (c>32 && c<127) putch(c);
            putch(' ');
            if (c=='.')
            {
                printf("MX115OFF ");
                RC5(0x15,0x0c,2); // Philips MC115 AUX OFF
            }
            else if (c=='n')
            {
                printf("NEC ");
                NEC(0x55,0xaa); 
            }
            else
            {
                putch('?');
                continue;
            }
            while (irsnd_is_busy ()) ;
            printf("PR2 %d\r\n",PR2);
            continue;
        }
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
