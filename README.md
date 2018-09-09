# Alarm_Clock_Using_ATMEGA328
## Overview:
This is a project where I designed and built a digital alarm clock using the Atmel ATMEGA328 microcontroller.
## Hardware:
The the main hardware used in this project include:
- Atmel ATMEGA328 microcontroller
- MAX7221 7-segment 8 digit LED driver
- 7-segmet LED display
- 32.768kHz crystal oscillator 
- Buttons, wires, battery, switch, etc.
### Hardware Explained:
- Atmel ATMEGA328

I used the popular Atmel ATMEGA328, which is one of the most popular Atmel AVR models and is used in the Arduino. This is a 28 pin microcontroller that has 32kB of program memory and 2kB of RAM. I used the microcontroller to interface with the MAX7221 LED driver using the SPI interface protocol.
- MAX7221 8 digit LED driver

This IC can control an 8x8 led matrix or an 8 digit 7-segment LED display and uses the Serial Peripheral Intercom (SPI) protocol. This chip is desigend to work with common-cathode LEDS. In this project, the MAX7221 took commands from the ATMEGA328 through the SPI protocol, to display the the current circumstances of the alarm clock onto the 7-segment LED dispaly.

- 7-segment LED display

This is where the time, alarm value or current circumstance of the alarm is displayed. 

-  32.768kHz crystal oscillator

The 32.768kHz crystal oscillator was used as the timing mechanism in the alarm clock. The crystal has an Equivalent Series Resistance (ESR) of 30 kOhms and load capacitance of 12.5 pF. The crystal has a precision of +- 20 parts per million. The crystal controls the frequency of an 8-bit counter on the ATMEGA328. The 32.768kHz frequency of the crystal divides exactly into 1 second intervals when the counter is configured to have a prescale of 64 and to overflow after the standard 256 ticks. The exact division of this crystal to one second and it's precision are why it was used to control the clock. 

## Update
- Total code revision
- Now 24 hour clock (A.M./P.M.)
- Changed LED Display
- A.M., P.M., :, Alarm Indicators
- Indicators need POT in series with resistor for brightness control
- Lots of cleaning needed in code

## Update 9/9
- P.M. to A.M. bug fixed. 
- Indictors for A.M. and P.M. work
- Alarm indicator is very dim, whicth seems to be a hardware issue, will try with another MCU to see if the PORT is damaged
- Alarm clock is working, needs testing and analysis
