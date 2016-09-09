/******************************************************************************

Test program IRMP for ESP8266                2015-11-16 Wolfgang Strobl, Bonn

$Id: irmp-main-esp8266.c,v 1.2 2016/01/12 21:15:16 fm Exp $

IRMP ported to ESP8266, testet with MOD-WIFI-ESP8266-DEV on
ESP8266-EVB evaluation board. https://www.olimex.com/Products/IoT/ESP8266-EVB/

Connections
-----------

Input TSOP via 1k resistor at GPIO12 (Pin 7 UEXT), 
Output via UART (Pin 3/4 UEXT)

example output
---------------

ESP8266 IRMP Test v0.3 W.Strobl 20151120
F_INTERRUPTS==15000
SDK version: 1.4.1(pre2) Chip ID=10619495
data  : 0x3ffe8000 ~ 0x3ffe837a, len: 890
rodata: 0x3ffe8380 ~ 0x3ffe891c, len: 1436
bss   : 0x3ffe8920 ~ 0x3ffef4c0, len: 27552
heap  : 0x3ffef4c0 ~ 0x3fffc000, len: 52032
free heap size=51784, system time=330392, rtc time=59472
IRMP listening ...
mode : sta(18:fe:34:a2:0a:67)
add if0

IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x23f1, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x1ffe, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x28fc, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x0113, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x28fc, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x09ff, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x28fc, f=0
IRMP TELEFUNKEN(34): addr=0x0000 cmd=0x0113, f=0
IRMP   KASEIKYO( 5): addr=0x2002 cmd=0x9001, f=0
IRMP   KASEIKYO( 5): addr=0x2002 cmd=0x9b40, f=0
IRMP      SIRCS( 1): addr=0x0809 cmd=0x1d0b, f=0
IRMP      SIRCS( 1): addr=0x0809 cmd=0x1d7a, f=0
IRMP      SIRCS( 1): addr=0x0809 cmd=0x1d7c, f=0
IRMP      SIRCS( 1): addr=0x0809 cmd=0x1d79, f=0
IRMP      SIRCS( 1): addr=0x0809 cmd=0x1d7c, f=0
IRMP    SAMSG32(10): addr=0x2d2d cmd=0xc639, f=0
IRMP    SAMSG32(10): addr=0x2d2d cmd=0xb54a, f=0

*******************************************************************************/

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"

#include "irmp.h"

// hardware timer (driven by NMI)

typedef enum {
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

void hw_timer_set_func (void (* user_hw_timer_cb_set)(void));

void hw_timer_init (
FRC1_TIMER_SOURCE_TYPE source_type,
u8 req)
;

void irmp_timer(void)
{
    irmp_ISR ();
}

// info

void meminfo(void)
{
    os_printf("free heap size=%u, system time=%u, rtc time=%u \n",
        system_get_free_heap_size(),
        system_get_time(),
        system_get_rtc_time());
}

void sysinfo(void)
{
    os_printf("SDK version: %s Chip ID=%u\n",
        system_get_sdk_version(),
        system_get_chip_id());
    system_print_meminfo();
    meminfo();
}

// Tasks

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

// unbuffered Uart-rx, based on a comment in
// https://github.com/SuperHouse/esp-open-rtos/issues/18

int my_rx_one_char(void)  // char or -1 
{
    int c = READ_PERI_REG(UART_STATUS(0)) & 0xff;
    if (c) return READ_PERI_REG(UART_FIFO(0));
    return -1;
}


IRMP_DATA irmp_data;

//------------------ User Task ---------------------

static void 
user_procTask(os_event_t *events)
{
    int rc = irmp_get_data (&irmp_data);   
      
    if (rc)
    {
        os_printf("\nIRMP %10s(%2d): addr=0x%04x cmd=0x%04x, f=%d ",
            irmp_protocol_names[ irmp_data.protocol],
            irmp_data.protocol,
            irmp_data.address,
            irmp_data.command,
            irmp_data.flags
        );
    }
    
    // https://github.com/SuperHouse/esp-open-rtos/issues/18
    // uart_rx_one_char ist offenbar eine ROM-Funktion.
    
    int c = my_rx_one_char();
    
    if(c != -1) 
    {
        uart_tx_one_char(0,c);
        os_printf("(0x%02x, %d) ",c,c);
        switch(c)
        {
            case '.':
                os_printf("\nTime=%d, GPIO12=%d, ",
                system_get_time(),GPIO_INPUT_GET(12));
                os_printf("gpio=%08x ",gpio_input_get());
                break;
        }
    }    
    os_delay_us(100);
    system_os_post(user_procTaskPrio, 0, 0 );    
}

// Init function 

void ICACHE_FLASH_ATTR
user_init()
{
    void* p;
    uint32 now,diff;
    
    //~ system_timer_reinit(); //US_TIMER
    
    uart_init(BIT_RATE_115200, BIT_RATE_115200);    
    os_printf("\n\nESP8266 IRMP Test v0.3 W.Strobl 20151120\n");

    os_printf("F_INTERRUPTS==%d\n",F_INTERRUPTS);
    
    sysinfo();
    
    hw_timer_init(NMI_SOURCE,1);
    hw_timer_set_func(irmp_timer);
    hw_timer_arm (1000000/F_INTERRUPTS);
    
    // Initialize the GPIO subsystem.
    gpio_init();
    
    
    irmp_init ();
    
    //Start os task
    
    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
    
    os_printf("IRMP listening ...\n");
        
}
