+#include <TimerOne.h>
/* first include Arduino.h, the IDE includes it after irmp*.h ... */
#include "Arduino.h"
/* ... and then chokes on uintX_t ... */

#include <irmp.h>
#include <irsnd.h>

/* undefine this if you don't want blinking LED for diagnosis */
#define LED_PIN 13
#define SER_BAUD 115200

/* F_INTERRUPTS is the interrupt frequency defined in irmpconfig.h */
#define US (1000000 / F_INTERRUPTS)
void setup()
{
    Serial.begin(SER_BAUD);
    delay(1000);
    /* greeting string and debugging ouput */
    Serial.println("IRMP test sketch");
    Serial.print("US: ");
    Serial.println(US);
    Serial.println("Send example: P:02 A:916E C:000F (NEC Taste 1)");
#ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
#endif
    irmp_init();
    irsnd_init();
    //sei();
    led(HIGH);
    delay(20); /* make sure the greeting string is out before starting */
    led(LOW);
    Timer1.initialize(US);
    Timer1.attachInterrupt(timerinterrupt);
}

IRMP_DATA irmp_data[3];
uint8_t act_data = 0;
int incomingByte = 0;   // for incoming serial data

void loop()
{
    IRMP_DATA* data = &irmp_data[act_data];
    if (irmp_get_data(data))
    {
        led(HIGH);
#if IRMP_PROTOCOL_NAMES == 1
        Serial.print(irmp_protocol_names[data->protocol]);
        Serial.print(" ");
#endif
        Serial.print("P:");
        Serial.print(data->protocol, HEX);
        Serial.print(" A:");
        Serial.print(data->address, HEX);
        Serial.print(" C:");
        Serial.print(data->command, HEX);
        Serial.print(" ");
        Serial.print(data->flags, HEX);
        Serial.println("");
        /* Serial.print is asynchronous, so the LED is only flashing very lightly */
        led(LOW);

        data->flags = 0;    // reset flags!
        int result = irsnd_send_data(data, TRUE);
        if (result != 1)
        {
            Serial.println("loop : irsnd_send_data ERROR");
        }
        else
        {
            if (++act_data >= 3)
            {
                act_data = 0;
            } 
        }
    }

    if (Serial.available() == 18 && readAndCheck('P') && readAndCheck(':'))
    {
        // read the protocol
        data->protocol = readHex() * 16 + readHex();

        if (readAndCheck(' ') && readAndCheck('A') && readAndCheck(':'))
        {
            // read the address
            data->address = ((readHex() * 16 + readHex()) * 16 + readHex()) * 16 + readHex();

            if (readAndCheck(' ') && readAndCheck('C') && readAndCheck(':'))
            {
                // read the address
                data->command = ((readHex() * 16 + readHex()) * 16 + readHex()) * 16 + readHex();

                // send ir data
                data->flags = 0;
                int result = irsnd_send_data(data, TRUE);
                if (result)
                {
                    Serial.print("Send IR data: ");
                    Serial.print("P:");
                    Serial.print(data->protocol, HEX);
                    Serial.print(" A:");
                    Serial.print(data->address, HEX);
                    Serial.print(" C:");
                    Serial.print(data->command, HEX);
                    Serial.println("");

                    if (++act_data >= 3)
                    {
                        act_data = 0;
                    } 
                }
            }
        }
    }
}

/* helper function: attachInterrupt wants void(), but irmp_ISR is uint8_t() */
void timerinterrupt()
{
#if 1   // if TSOP receiver can't detect my own IR LED, call both functions:
    irsnd_ISR();                        // call irsnd ISR
    irmp_ISR();                         // call irmp ISR
#else   // if TSOP receiver also detects my own IR LED, don't receive while sending:
    if (! irsnd_ISR())                  // call irsnd ISR
    {                                   // if not busy...
        irmp_ISR();                     // call irmp ISR
    }
#endif
}

static inline void led(int state)
{
#ifdef LED_PIN
    digitalWrite(LED_PIN, state);
#endif
}

static inline int readAndCheck(int c)
{
    return c == Serial.read();
}

static inline int readHex()
{
    int c = Serial.read();
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }

    c |= 0x20; // lowercase

    if (c >= 'a' && c <= 'f')
    {
        return c + 10 - 'a';
    }

    return -1;
}
