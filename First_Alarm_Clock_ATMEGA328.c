/* Alarm_Clock_ATMEGA328.c */
// Newest version with current Hardware

// PIN 16) PB2 = SS = CS    //Used to be MOSI on old code
// CS = active low
// PIN 17) PB3 = MOSI = DIN on 7-segment driver
// PIN 19) PB5 = SCK = CLK

// copied from Git folder
// Use new indicator with MCU pin and Resistor to ground, SPI is slower, 
// : (), A.M. Indicator, P.M. Indicator, ' Alarm indicator {Digit 4}
// 7-Segment Display (Pin 7 = A.M., Pin 8 = P.M., Pin 6 = :, Pin 16 = Alarm
//  Maximum current per LED segment = 25 mA on large clock display
// Want pot in series with resistor for indicators and 7-Segment driver 

// PC0 = A.M., PC1 = P.M., PC2 = :
// PD7 = Alarm indicator

// Adjust resistors and POTs, Warning: On lowest Intensity

// Make: LED blink when pressing enter during time and alarm set

#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// could use #define for Indicators

void send_byte(int byte_of_data);
void send_command(int address_byte, int data_byte);
void blank_display(void);
void set_time(void);


volatile int time_count;    // counts time in number of minuets
volatile int alarm_time_count;

uint8_t is_time_set_yet;
uint8_t OCR0A_going_down=1;    //Alarm sounds stuff, prob volatile 

volatile uint8_t is_alarm_set;
volatile uint8_t has_time_been_set;
volatile uint8_t was_time_just_set;
volatile uint8_t second_count;
volatile uint8_t hours;    // time values might not need to be volatile
volatile uint8_t min;      // but I wanted to be safe working between interrupts

volatile uint8_t hours_24_format;
volatile uint8_t alarm_hours;
volatile uint8_t alarm_min;
volatile uint8_t alarm_hours_24;

int main(void)
{
	// Data Direction
	DDRB |= (1<<DDB3) | (1<<DDB2) | (1<<DDB5);    //all output for PORTB except pb6 and pb7 for oscillator
	DDRC = 0x07;    // 0x07 = 0b00000111, PC0, PC1, PC2 = Indicators
	DDRD = 0xC0;    // 0xC0 = 0b11000000, DDD6 is 1 for OC0A output (alarm sound), NOW D7 is ' indicator

	// Port Initial Conditions
	PORTB |= (1<<PORTB2) | (1<<PORTB5);    //CS HIGH and CLK start HIGH

	// Crystal and Timer
	//TCCR2B |= (1<<CS20);    // Pre-Sale = 1, use for Debugging
	ASSR |= (1<<AS2);    //enable crystal as input for timer2
	TCCR2B |= (1<<CS22) | (1<<CS20);    //Pre-scale of 128

	// Interrupts
	EIMSK |= (1<<INT0) | (1<<INT1);    // enable int0, int1 interrupts, normally high, interrupts set when button press makes input low
	PCICR |= (1<<PCIE2);    //Enables PCINT16, PD0 as set_time interrupt
	PCMSK2 = 0x01;    // only PCINT16 interrupt should be enabled
	sei();
		
	//  Initalize 7-Segment LED Driver
	send_command(0x09, 0x0F);    //Decode mode for digits 0-3 (0xFF is for all digits)
	send_command(0x0A, 0x00);    //Intensity bottom
	send_command(0x0B, 0x04);    //Scan digits (0,1,2,3,4),0,1 = min, 2,3 = hours digit4 = indicators
	send_command(0x0C, 0x01);    //Turn on display

	// Blank Screen then Initalize Values for hours and min
	blank_display();
	
	PORTC |= (1<<PORTC2);    // Turn On :
	set_time();    // Initalize: time_value, hours, min and display
	has_time_been_set = 1;
	
	while (1)
	{
		// Alarm bug fix that allows alarm set for 12:00 to work
		if (time_count == alarm_time_count && is_alarm_set ==1){
			//is_alarm_set==1 is to make sure alarm does not go off at 12:00
			TCCR0A |= (1<<WGM01);
			TIMSK0 |=(1<<OCIE0A);
			OCR0A = 30;
			TCCR0B |= (1<<CS00) | (1<<CS01);
			is_alarm_set++;  //otherwise this gets stuck in loop;
		}
		
		// LED indicators for A.M., needs if statement
		PORTC |= (1<<PORTC0);    // Turn on A.M. LED
		PORTC &= ~(1<<PORTC1);    // Turn off P.M.
		
		second_count = 0;
		
		while (time_count < 720) {    // 720 minutes in 12 hours
			while (second_count<60) {
				if (TIFR2 & (1<<TOV2)) {
					TIFR2 |= (1<<TOV2);
					second_count++;
				}
			}
			second_count=0;
			time_count++;
			min++;
			if (min == 60){
				min = 0;
				hours++;
			}
				
			send_command(0x01, min%10);    // sets min digits after every 60s
			send_command(0x02,min/10);
			
			// Checks if alarm is set and equal to current time (every 60s)
			if (time_count == alarm_time_count && is_alarm_set == 1){
				TCCR0A |= (1<<WGM01);
				TIMSK0 |=(1<<OCIE0A);
				OCR0A = 30;
				TCCR0B |= (1<<CS00) | (1<<CS01);
				is_alarm_set++;  //otherwise this gets stuck in loop;
			}
			
			// Displays/update hour digits (every 60s)
			if (hours==0 || hours==12){    //moved 2 digits
				hours = 0;    // Might be able to delete
				send_command(0x03, 2);
				send_command(0x04, 1);
			}
			else if (hours<10){
				send_command(0x03,hours);
				send_command(0x04,0x0F);
			}
			else if (hours<12){
				send_command(0x03, hours%10);
				send_command(0x04,1);
			}
		}    // end of if time_count<720 (end A.M.)
		
		// LED Indicators for P.M.
		PORTC &= ~(1<<PORTC0);    // Turn OFF A.M. LED
		PORTC |= (1<<PORTC1);    // Turn ON P.M.
		
		// Alarm bug fix that allows alarm set for 12:00 to work
		if (time_count == alarm_time_count && is_alarm_set ==1){
			TCCR0A |= (1<<WGM01);
			TIMSK0 |=(1<<OCIE0A);
			OCR0A = 30;
			TCCR0B |= (1<<CS00) | (1<<CS01);
			is_alarm_set++;  //otherwise this gets stuck in loop;
		}
		
		// Bug fix
		hours = hours%12;
		was_time_just_set = 0;
		
		// P.M. 12 hour Loop
		while (time_count >= 720) {    // 1440 min in 24 hours
			while (second_count<60) {
				if (TIFR2 & (1<<TOV2)) {
					TIFR2 |= (1<<TOV2);
					second_count++;
				}
			}
			second_count=0;
			time_count++;
			min++;
			if (min == 60){
				if (time_count == 1440) {    // maybe will cause A.M. values to go to time_count<720 loop
				break;
				}
				min = 0;
				hours++;
			}
			
			send_command(0x01, min%10);    // sets min digits after every 60s
			send_command(0x02,min/10);
			
			// Checks if alarm is set and equal to current time (every 60s)
			if (time_count == alarm_time_count && is_alarm_set == 1){
				//is_alarm_set==1 is to make sure alarm does not go off at 12:00
				TCCR0A |= (1<<WGM01);
				TIMSK0 |=(1<<OCIE0A);
				OCR0A = 30;
				TCCR0B |= (1<<CS00) | (1<<CS01);
				is_alarm_set++;  //otherwise this gets stuck in a weird loop;
				
			}
			
			// Displays/update hour digits (every 60s)
			if (hours==0 || hours==12){    //moved 2 digits
				hours = 0;    // Might be able to delete
				send_command(0x03, 2);
				send_command(0x04, 1);
			}
			else if (hours<10){
				send_command(0x03,hours);
				send_command(0x04,0x0F);
			}
			else if (hours<12){
				send_command(0x03, hours%10);
				send_command(0x04,1);
			}
		}    // end of if time_count<1440 (end P.M.)
		
		// Allows time to be P.M, then set to A.M. without making time_count = 0
		if (was_time_just_set != 1) {
		time_count = 0;
		hours = 0;
		min = 0;
		was_time_just_set=0;
		}
		
	}
}

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

void send_command(int address_byte, int data_byte){
	PORTB &= ~(1<<PORTB2);    //CS LOW
	send_byte(address_byte);
	send_byte(data_byte);
	PORTB |= (1<<PORTB2);    //CS high
}

void blank_display(void){
	for (int i=1; i<5; i++){
		send_command(i, 0x0F);
	}
}


// Changes: made hours and min values by button press be a local variable to set_time(), assign hours = (hours_24%12) to allows same display code and maybe pass vairables easier
void set_time(void){
	
	time_count = 0;
	hours_24_format = 0;
	hours = 0;
	min = 0;
	blank_display();	
	
	while ((PINC & (1<<PORTC3))) {    // While P3 is High, goes low on button press

		send_command(0x01,0x00);    // Alarm Bug fix //did not changes these 2 lines
		send_command(0x02,0x00);    // Alarm Bug fix
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			hours_24_format++;
			if (hours_24_format == 24) {
				hours_24_format = 0;
			}
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (hours_24_format == 0){
				hours_24_format = 24;
			}
			hours_24_format--;
			_delay_ms(250);
		}
		
		// control LEDs
		if (hours_24_format < 12) {
			PORTC |= (1<<PORTC0);    // A.M. Indicator = ON
			PORTC &= ~(1<<PORTC1);    // P.M. Indicator OFF
		}
		else {
			PORTC &= ~(1<<PORTC0);    // A.M. Indicator = OFF
			PORTC |= (1<<PORTC1);    // P.M. Indicator ON
		}
		
		hours = (hours_24_format%12);
		
		if (hours==0 || hours==12){
			//hours = 0;    // keeps hours between 0 and 11
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		else if (hours<10 && hours!=0){
			send_command(0x03,hours);
			send_command(0x04,0x0F);
		}
		else if (hours == 10){
			send_command(0x03, 0x00);
			send_command(0x04,0x01);
		}
		else if (hours==11){
			send_command(0x03, hours%10);
			send_command(0x04,0x01);
		}
	}

	send_command(0x01,0x0F);    //Blank min briefly when set, blanking hours looked weird to me
	send_command(0x02,0x0F);

	_delay_ms(500);

	while ((PINC & (1<<PORTC3))){
		
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			min++;
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (min == 0){
				min = 60;
			}
			min--;
			_delay_ms(250);
		}
		if (min<60){
			send_command(0x01, min%10);
			send_command(0x02,min/10);
		}
		
		else {
			send_command(0x01,0x00);    // while setting the time if increment min was held display would not show min at 00, would go from 59, pause then to 01
			send_command(0x02,0x00);
		}
		
		if (min == 60){    // New Stuff, was(min == 61), now(min==60), i think when setting the time going upward 00 would be displayed twice
			min = 0;
		}
		if (hours==0 || hours==12){    // Alarm bug fix beginning
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		else if (hours<10 && hours!=0){
			send_command(0x03,hours);
			send_command(0x04,0x0F);

		}
		else if (hours == 10){
			send_command(0x03, 0x00);
			send_command(0x04,0x01);
		}
		else if (hours==11){    // Last segment of alarm bug fix
			send_command(0x03, hours%10);
			send_command(0x04,0x01);
		}    // End of alarm bug fix
		

	}
	time_count = (hours_24_format * 60);    // This was the same but it was: time_count = (hours * 60);
	time_count = time_count + min;
	
	is_time_set_yet = 1;
	blank_display();
	_delay_ms(500);
	//re-set display to correct time

	// min
	send_command(0x01, min%10);
	send_command(0x02,min/10);
	// hours
	if (hours==0 || hours==12){
		//hours = 0;
		send_command(0x03, 2);
		send_command(0x04, 1);
	}
	else if (hours<10){
		send_command(0x03,hours);
		send_command(0x04,0x0F);
	}
	else if (hours<12){
		send_command(0x03, hours%10);
		send_command(0x04,1);
	}
	second_count = 0;
	_delay_ms(1000);
	
	//one alarm bug
	//alarm goes off if set to alarm time(no wait)
	if ((time_count == alarm_time_count) && is_alarm_set ==1){    //is_alarm_set==1 is to make sure alarm does not go off at 12:00
		TCCR0A |= (1<<WGM01);
		TIMSK0 |=(1<<OCIE0A);    //Timer for alarm sound interrupt enabled
		OCR0A = 30;
		TCCR0B |= (1<<CS00) | (1<<CS01);
		is_alarm_set++;  //otherwise this gets stuck in a weird loop;
	}
	// New 9/4
	// allow for P.M. time to be able to be set to A.M. time
	if (has_time_been_set == 1) {
		was_time_just_set = 1;
	}
	else {
		was_time_just_set=0;
	}
	
	// New Bug Fix: Seconds = 60, time_count--, min--: will end min loop, go right to time_count++, seconds = 0, min ++
	if (has_time_been_set == 1) {
		second_count = 60;
		time_count--;
		min--; 
	}
	return;
	// when return to main, main will be busy cycling through seconds to count the next minute, so, display need to reset display to new value of the time being set
}



ISR(INT0_vect)    //set alarm
{
	blank_display();
	PORTD |= (1<<PORTD7);    // Turn on Alarm Indicator
	
	alarm_time_count = 0;
	alarm_hours = 0;
	alarm_min = 0;
	alarm_hours_24 = 0;
	
	
	while ((PINC & (1<<PORTC3))) {    // While P3 is High, goes low on button press
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			alarm_hours_24++;
			if (alarm_hours_24 == 24) {
				alarm_hours_24 = 0;
			}
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (alarm_hours_24 == 0){
				alarm_hours_24 = 24;
			}
			alarm_hours_24--;
			_delay_ms(250);
		}
		
		if (alarm_hours_24 < 12) {
			PORTC |= (1<<PORTC0);    // A.M. Indicator = ON
			PORTC &= ~(1<<PORTC1);    // P.M. Indicator OFF
		}
		else {
			PORTC &= ~(1<<PORTC0);    // A.M. Indicator = OFF
			PORTC |= (1<<PORTC1);    // P.M. Indicator ON
		}
		
		alarm_hours = (alarm_hours_24%12);
		
		if (alarm_hours < 0 || alarm_hours > 12 ){    // Might be unnecessary if alarm_hours n not allowed out of range
			send_command(0x03,0x0F);
			send_command(0x04,0x0F);
		}
		if (alarm_hours==0 || alarm_hours==12){
			//hour_count = 0;
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		if (alarm_hours<10 && alarm_hours!=0){
			send_command(0x03,alarm_hours);
			send_command(0x04,0x0F);
		}
		if (alarm_hours == 10){
			send_command(0x03, 0x00);
			send_command(0x04,0x01);
		}
		if (alarm_hours==11){
			send_command(0x03, alarm_hours%10);
			send_command(0x04,0x01);

		}			
	}
	_delay_ms(500);

	while ((PINC & (1<<PORTC3))){
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			alarm_min++;
			_delay_ms(250);
		}
		if (alarm_min == 60){
			alarm_min = 0;
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (alarm_min == 0){
				alarm_min = 60;
			}
			alarm_min--;
			_delay_ms(250);
		}
		if (alarm_min<60){
			send_command(0x01, alarm_min%10);
			send_command(0x02,alarm_min/10);
		}
		
		if (alarm_min < 0){    //Should be unnecessary
			send_command(0x02, 0x00);
			send_command(0x01, 0x00);
		}
	}
	_delay_ms(1000);    // maybe delete
	blank_display();
	//resets hours and min so time is displayed
	if (hours<10){    //these have been moved 2 digits to the left
		send_command(0x03, hours%10);
		send_command(0x04, 0x0F);
	}
	if (hours<12 && hours>9){
		send_command(0x03, hours%10);
		send_command(0x04,0x01);
	}
	if (hours == 12 || hours == 0){
		send_command(0x03, 0x02);
		send_command(0x04, 0x01);
	}
	send_command(0x01, min%10);
	send_command(0x02,min/10);
	if (is_time_set_yet==0){    // ?????
		blank_display();
	}
	
	is_alarm_set = 1;
	alarm_time_count = (alarm_hours_24 * 60);
	alarm_time_count = alarm_time_count + alarm_min;
	
	// A.M. and P.M. Led indicators for time
	if (time_count < 720) {
		PORTC |= (1<<PORTC0);    // A.M. Indicator = ON
		PORTC &= ~(1<<PORTC1);    // P.M. Indicator OFF
	}
	else {
		PORTC &= ~(1<<PORTC0);    // A.M. Indicator = OFF
		PORTC |= (1<<PORTC1);    // P.M. Indicator ON
	}
	return;
}

ISR (TIMER0_COMPA_vect)    // Alarm sound
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

ISR(INT1_vect)    //Turn off all alarm stuff
{
	TCCR0B = 0x00;    // Turns off Speaker
	PORTD &= ~(1<<PORTD7);    // Turn OFF LED
	is_alarm_set = 0;
	return;
	
}


ISR(PCINT2_vect)    //Interrupt to call set_time() on PIN2 (PD0)
{
	blank_display();
	_delay_ms(1000);
	set_time();
	PCIFR |= (1<<PCIF2);    // maybe get rid of double interrupt
	return;
}
