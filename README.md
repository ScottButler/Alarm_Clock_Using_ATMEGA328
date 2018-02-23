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

The 32.768kHz crystal oscillator was used as the timing mechanism in the alarm clock. The crystal has an Equivalent Series Resistance (ESR) of 30 kOhms and load capacitance of 12.5 pF. The crystal has a precision of +- 20 parts per million. The crystal controls a counter on the ATMEGA328 and this counter is divided by the correct counter pre-scaler of an 8-bit counter divided exactly to one second for every counter overflow. The exact division of this crystal to one second and it's precision are why it was used to control the clock. 

## The Program:
