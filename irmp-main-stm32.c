/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp-main-stm32.c - demo main module to test IRMP decoder on STM32
 *
 * Copyright (c) 2009-2015 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmp-main-stm32.c,v 1.1 2016/01/12 11:55:05 fm Exp $
 *
 * This demo module is runnable on STM32
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

uint32_t
SysCtlClockGet(void)
{
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    return RCC_ClocksStatus.SYSCLK_Frequency;
}

void
timer2_init (void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_ClockDivision                 = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode                   = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period                        = 7;
    TIM_TimeBaseStructure.TIM_Prescaler                     = ((F_CPU / F_INTERRUPTS)/8) - 1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                      = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd                   = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 0x0F;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void
TIM2_IRQHandler(void)                                                       // Timer2 Interrupt Handler
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  (void) irmp_ISR();                                                        // call irmp ISR
  // call other timer interrupt routines...
}

int
main (void)
{
    IRMP_DATA irmp_data;
        
    irmp_init();                                                            // initialize irmp
    timer2_init();                                                          // initialize timer2

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
