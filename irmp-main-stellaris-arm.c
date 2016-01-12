/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-stellaris-arm.c - demo main module to test IRMP decoder on LM4F120 Launchpad (ARM Cortex M4)
 *
 * Copyright (c) 2009-2016 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmp-main-stellaris-arm.c,v 1.1 2016/01/12 11:55:05 fm Exp $
 *
 * This demo module is runnable on LM4F120 Launchpad (ARM Cortex M4)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include "irmp.h"

#ifndef F_CPU
#error F_CPU unknown
#endif

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
