# Alarm_Clock_Using_ATMEGA328
## Overview:
This is a project where I designed and built a digital alarm clock using the Atmel ATMEGA328 microcontroller.
## Features:
- 24 hour based alarm clock
- A.M./P.M. and Alarm LED indicators
- Horrible alarm sound
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

# Alarm_Clock_ATMEGA3228.c Code
## How the program works
### Program Overview:
The Atmega328 microcontroller uses an external crystal oscillator that oscillates at 32.768 kHz as the clock for an 8-bit timer counter. The 8-bit timer takes 256 counts to overflow. A pre-scaler of 128 is applied to this counter, meaning every timer overflow occurs when the counter clicks 32,768 times—or every one second.  A timer overflow interrupt is triggered on every timer overflow where two global variables are manipulated. One global variable is called second_count, this variables value is increments on every timer overflow. Every time the value of second_count is 60 a second global variable, time_count, is incremented.  Inside the main function, using the time_value global variable, values for the hour and minute value for the current time is calculated. The Atmega328 communicates with a MAX7221 LED driver that controls the segment LED clock display, and displays the time. 
### Functions:
#### SPI
- First, I will discuss the SPI functions.
- I know there are built in SPI functions on this microcontroller, but I wanted to understand the SPI protocol better so I wrote a “bit-banged” version than manually does SPI communication. 
- Here are the Functions and a description of them:
```
void send_byte(int byte_of_data){
	for (int i=0; i<8; i++, byte_of_data<<=1){
		PORTB &= ~(1<<PORTB5);    //Clock LOW
		if (byte_of_data & 0x80){    //If most significant bit is high, 0x80 = 0b10000000
			PORTB |= (1<<PORTB3);    // MOSI HIGH
		}
		else {
			PORTB &= ~(1<<PORTB3);    //MOSI LOW
		}
		PORTB |= (1<<PORTB5);    //Clock HIGH
	}
}
```
- This function takes an 8-bit value as an argument. The clock line is pulled down. Next, if the most significant bit in the argument is 1, the MOSI pin if set high, otherwise it is set low. Then, the clock line is pulled low. The argument is left adjusted with the << operator, and the process repeats for all 8-bits. 
-NOTE: the argument could be of type uint8_t, but other variables may be required so that the 8-bit data types do not overflow. But, the current configuration works fine, and possible wasted clock cycles do not matter in this situation. 
```
void send_command(int address_byte, int data_byte){
	PORTB &= ~(1<<PORTB2);    //CS LOW
	send_byte(address_byte);
	send_byte(data_byte);
	PORTB |= (1<<PORTB2);    //CS high
}
```
- The CS pin is driven low. When the CS pin is in its active low state first the address byte is sent, then the data byte is sent. Then after the 2 bytes are transmitted the CS pin is pulled high and transmission is terminated. 
#### Blank Display
```
void blank_display(void);
```
- This function sends the command the byte of data, `0x0F`, to each digit on the segment LED display. The value 0x0F is the command to blank the display as noted in the MAX7221 datasheet. 
 #### Set Time
```
void set_time(int *hours, int *min);
```
- This function sets the time for the alarm clock
- Inside the function the global variable time_value is set by calculating how many minutes past 12:00 A.M. there are for the time that was set.
- The arguments are pointers. This is because the values of the arguments will be used in the main function. The addresses of the arguments are the parameters of the function call, like this:
```
set_time(&hours, &min);
```
- This is the pass by reference technique that allows for the arguments to be dereferenced and have their value changed. 
- The values for hours and min are then used in main to display the time. If these variables were instead passed by value, there is a glitch in the display then the time is changed. Since hours and min are updated in main the display glitch is very brief, but still unpleasant to look at. 
- NOTE: could make hours and min uint8_t types, but may need more variables to avoid overflow during math.
#### Set Alarm
```
void set_alarm(int *alarm_time_count, uint8_t *is_alarm_set);
```
- This function sets the time the alarm should go occur
- is_alarm_set is a variable that prevents the alarm to go off if an alarm has not been set
- alarm_time_count is the number of minutes after 12:00 A.M. that the alarm is set. 
- The alarm goes off when the following condition is true in the code below:
```
if ((time_count == alarm_time_count) && is_alarm_set == 1){
…
}
```
- If there was no is_alarm_set variable, if the alarm was never set, at 12:00 A.M. the alarm would go off. This is because alarm_time_count is initialized to zero. Also, if the variable did not exist, if the alarm was set, every 24 hours time_count would be equal to alarm_time_count, and the alarm would go off. 
-This function passes the addresses of the variables as the arguments, so that the values may be modified inside the function, and the new modified values may be used in main.
- This is how this function is called:
```
set_alarm(&alarm_time_count, &is_alarm_set);
```
#### Turn Off Alarm
```
void turn_off_alarm(uint8_t *is_alarm_set);
```
- Simple function that turns off the alarm indicator LED, turns off the counter that produces the alarm sound, and makes is_alarm_set equal zero. 

### How time is kept:
A 32.768 kHz crystal oscillator is used as the clock source for an 8-bit counter. Timer2 is has the crystal as the clock source and a pre-scale of 128 so that it takes 32.768 ticks for timer 2 to overflow. The crystal has an accuracy of +- 20 parts per million. A timer over flow interrupt executed on one second intervals. 
-Here is the code for the timer overflow interrupt and a discussion of how what it is doing:
```
ISR (TIMER2_OVF_vect)
{
	second_count++;
	if (second_count == 60) {
		time_count++;
		second_count = 0;
	}
	if (time_count >= 1440) {
		time_count = 0;
	}
}
```
- The first global variable, second_count, is incremented every second.
- Every 60 seconds, the second global variable time_count, is incremented and second_count is set to zero, starting a new minute in time. 
- The last if condition says if time_count is greater or equal to 1440, set tme_count to zero. There are 1440 minutes in a day, so this make 24 hour loops possible. 
- NOTE: In ``` if (time_count >= 1440)``` ,  greater than or equal is used as a protection incase time_count get corrupted and somehow becomes larger than 1440 without ever equaling 1440. 
### The Alarm:
The alarm is a very strange sound, and it is intended to be an unsettling sound.
- Here is the code that controls the alarms function inside main:
```
if ((time_count == alarm_time_count) && is_alarm_set == 1){
	TCCR0A |= (1<<WGM01);
	TIMSK0 |=(1<<OCIE0A);
	OCR0A = 30;
	TCCR0B |= (1<<CS00) | (1<<CS01);
	is_alarm_set++;  //otherwise this gets stuck in loop;
}
```
- The if statement tests if the alarm time matches with the current time, and also if the alarm has been set.
- If the conditions are true, timer0, an 8 bit counter/timer, is initialized. 
- The timer is in CTC mode with a pre-scale of 64. So, when the value OCR0A can vary to change the frequency of the waveform generated by the counter. 
- The variable OCROA_going_down is manipulated inside the timer0 output compare interrupt in the following code:
```ISR (TIMER0_COMPA_vect)
{
	if (OCR0A >= 30 && OCR0A_going_down%2 != 0){
		if (OCR0A ==30){
			OCR0A_going_down++;
		}
		OCR0A--;
	}
	
	if (OCR0A <=200 && OCR0A_going_down%2==0){
		if (OCR0A==200){
			OCR0A_going_down++;
		}
		OCR0A++;
	}
	PORTD ^= (1<<PORTD6);
	return;
}
```
- Here it is shown that the value of OCR0A increases and decreases periodically, changing the frequency very quickly.
- The values for OCR0A were choses to make the alarm sound to be strange and act as an alarm that the user would want to end quickly. 
### Main Function:
- Values for hours and min are caluclated and then displayed
- A.M./P.M. LED indicators are updated
- Scans for button presses to:
  - Set time
  - Set alarm
  - Turn off alarm
### Why two separate programs
- This will be discussed later, along with test results and edits to readme
- Stay tuned for more…

### Video
- I added a video that shows a proof of concept demo of the alarm functionality
- Video has low audio quality and acoustics, so the alarm sound is not as loud as in person.
- To hear the alarm sound, turn the volume up on your computer.
## Testing
-I compared the time on my alarm clock with a Sony alarm clock that I have on my desk.
- I started my clock at the same time displayed on the Sony as close as I could on a minute transition  
## Testing
- I compared the time on my alarm clock with a Sony alarm clock that I have on my desk.
- I started my clock at the same time displayed on the Sony as close as I could on a minute transition, noted how closely the clocks times match up, and then watched for deviations of the times displayed on both clocks.
####Result:
- After about one week, my clock was between half a second and one second faster than the Sony.
- So the time for my clock to be one minute faster than the Sony clock would be, roughly, between 1 and 2 years. 
Note: I need to run this experiment more times and for longer durations to get a more precise result. 


