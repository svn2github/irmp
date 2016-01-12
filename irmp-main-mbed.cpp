/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-mbed.cpp - demo main module to test IRMP decoder on AVR
 *
 * $Id: irmp-main-mbed.cpp,v 1.1 2016/01/12 11:55:05 fm Exp $
 *
 * This demo module is runnable on MBED boards
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include "mbed.h"
#include "irmp.h"

#define LED_ON  0
#define LED_OFF 1

DigitalOut led(P0_14, 1);
DigitalOut flash(P0_12, 1);

Ticker t;

// only for performance test
Timer   timerPerfTest;
int     timeISRMax = 0;
float   timeISRAvg;
int     timeISRAvgSum = 0;
int     countISRCalls = 0;

void irmpISR(void)
{
    int t1 = timerPerfTest.read_us();

    irmp_ISR();                                                             // call irmp ISR

    int timeISR = timerPerfTest.read_us() - t1;                             // calc time spent in worker ISR
    if (timeISR > timeISRMax)                                               // store maximum
    {
        timeISRMax = timeISR;
    }
    timeISRAvgSum += timeISR;                                               // sum for avg
    countISRCalls++;
}

int main()
{
    printf("IRMP on mbed\n");

    led = LED_OFF;
    timerPerfTest.start();

    IRMP_DATA irmp_data;

    irmp_init();                                                            // initialize irmp
    t.attach_us(&irmpISR, 1E6 / F_INTERRUPTS);                              // call ISR 15000/s

    // infinite loop, interrupts will toggle PORTD pins and handle UART communications.
    while (1)
    {
        flash = !flash;

        if (irmp_get_data (&irmp_data))
        {
            // ir signal decoded, do something here...
            // irmp_data.protocol is the protocol, see irmp.h
            // irmp_data.address is the address/manufacturer code of ir sender
            // irmp_data.command is the command code
            // irm_data.flags is press/release information
            // irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
            // printf("proto %d addr %d cmd %d\n", irmp_data.protocol, irmp_data.address, irmp_data.command );

            // sample decoding, toggle LED
            if (irmp_data.protocol == IRMP_RC5_PROTOCOL && irmp_data.address == 5)          // old RC5 VCR Remote. TV uses address 0
            {
                if (irmp_data.flags == 0)                                   // switch only on button press
                {
                    switch (irmp_data.command)
                    {
                        case 0:                                             // Key '0'
                            led = LED_OFF;
                            break;
                        case 1:                                             // Key '1'
                            led = LED_ON;
                            break;
                        case 53:                                            // Key 'play'
                            printf("bring me a beer!\n");
                            break;
                        case 54:                                            // Key 'stop'
                            timeISRAvg = (float)timeISRAvgSum / countISRCalls;
                            timeISRAvgSum = 0;
                            countISRCalls = 0;
                            printf("ISR max / avg runtime [microseconds] : %d / %5.2f\n", timeISRMax, timeISRAvg);
                            timeISRMax = 0;
                            break;
                    }
                }
            }

            // log to stdout
            printf("proto %d addr %d cmd %d flags %x name %s\n", irmp_data.protocol, irmp_data.address, irmp_data.command, irmp_data.flags, irmp_protocol_names[irmp_data.protocol] );
        }
    }
}
