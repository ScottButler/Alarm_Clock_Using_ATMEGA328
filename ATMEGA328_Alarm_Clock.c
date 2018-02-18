
// PIN 16) PB2 = SS = CS    //Used to be MOSI on old code
// CS = active low
// PIN 17) PB3 = MOSI = DIN on 7-segment driver
// PIN 19) PB5 = SCK = CLK


#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void send_byte(int byte_of_data);
void send_command(int address_byte, int data_byte);
void blank_display(void);
void set_time(void);

volatile int time_count = 0;    // counts time in number of minuets
// Use this to control time, min = time_count%60, hours = (time_count/60)
//have (60*12) mins be counted in main for 12 hours total

int is_time_set_yet = 0;
int is_alarm_set = 0;
int OCR0A_going_down=1;    //Alarm sounds stuff

int second_count = 0;
volatile int hours=0;    // time values might not need to be volatile
volatile int min=0;      // but I wanted to be safe working between interrupts
volatile int alarm_hours = -1;
volatile int alarm_min = -1;


int main(void)
{

	DDRD &= ~(1<<DDD0);    //Set PD0 as input pin for set_time() interrupt
	PCICR |= (1<<PCIE2);    //Enables PCINT16, PD0 as set_time interrupt
	
	PCMSK2 = 0x01;    // only PCINT16 interrupt should be enabled

	DDRD |= (1<<DDD6);    //OC0A as output, PD6, pin12
	
	DDRC = 0x00;
	DDRB |= (1<<DDB3) | (1<<DDB2) | (1<<DDB5);    //all output for PORTB except pb6 and pb7 for oscillator
	
	PORTB |= (1<<PORTB2) | (1<<PORTB5);    //CS HIGH

	ASSR |= (1<<AS2);    //enable crystal as input for timer2
	TCCR2B |= (1<<CS22) | (1<<CS20);    //Pre-scale of 128
	//TCCR2B |=(1<<CS20);    //pre-scale = 1, used for testing
	EIMSK |= (1<<INT0) | (1<<INT1);    // enable int0, int1 interrupts, normally high, interrupts set when button press makes input low
	sei();
	
	
	send_command(0x09, 0x0F);    //Decode mode for digits 0-3 (0xFF is for all digits)
	send_command(0x0A, 0x00);    //Intensity at bottom
	send_command(0x0B, 0x03);    //Scan digits (0,1,2,3)
	send_command(0x0C, 0x01);    //Turn on display
	
	blank_display();
	set_time();
	
    while (1) 
    {
		
		// Alarm bug fix that allows alarm set for 12:00 to work
		if (hours==alarm_hours && min == alarm_min && is_alarm_set ==1){
			//is_alarm_set==1 is to make sure alarm does not go off at 12:00
			TCCR0A |= (1<<WGM01);
			TIMSK0 |=(1<<OCIE0A);    //maybe OCI0B
			OCR0A = 30;
			TCCR0B |= (1<<CS00) | (1<<CS01);
			is_alarm_set++;  //otherwise this gets stuck in a weird loop;
		}
		
		second_count = 0;
		while (time_count < 720) {    // 720 minutes in 12 hours
			while (second_count<60){
				if (TIFR2 & (1<<OCF2A)) {
					TIFR2 |= (1<<OCF2A);
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
			if (hours==alarm_hours && min == alarm_min && is_alarm_set ==1){
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
		}    // end of if time_count<720 (end 12 hours)
		
		time_count=0;    //resets time_count<720 loop (12 hour loop)
		min = 0;    //resets time being displayed to 12:00 at beginning of 12 hour loop
		hours = 0;

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


void set_time(void){
	
	hours = 0;
	min = 0;
	blank_display();
	
	while ((PINC & (1<<PORTC3))) {    // While P3 is High, goes low on button press

		send_command(0x01,0x00);    // Alarm Bug fix //did not changes these 2 lines
		send_command(0x02,0x00);    // Alarm Bug fix
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			hours++;
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (hours == 0){
				hours = 12;
			}
			hours--;
			_delay_ms(250);
		}

		
		if (hours==0 || hours==12){
			hours = 0;    // keeps hours between 0 and 11
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		if (hours<10 && hours!=0){
			send_command(0x03,hours);
			send_command(0x04,0x0F);
		}
		if (hours == 10){
			send_command(0x03, 0x00);
			send_command(0x04,0x01);
		}
		if (hours==11){
			send_command(0x03, hours%10);
			send_command(0x04,0x01);
		}
		if (hours == 13){
			hours = 1;
		}
	}
	//New Stuff
	send_command(0x01,0x0F);    //Blank min briefly when set, blanking hours looked weird to me
	send_command(0x02,0x0F);
	//End New Stuff
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
		//New Stuff
		else {
			send_command(0x01,0x00);    // while setting the time if increment min was held display would not show min at 00, would go from 59, pause then to 01
			send_command(0x02,0x00);
		}
		//End new stuff
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
	time_count=0;    // might fix weird alarm bug
	int hour_number = hours*60;
	time_count = time_count+hour_number;    //sets time_count value to get to right spot of main
	time_count = time_count+min;

	is_time_set_yet = 1;
	blank_display();
	_delay_ms(500);
	//re-set display to correct time

	// min
	send_command(0x01, min%10);
	send_command(0x02,min/10);
	// hour
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
	if (hours==alarm_hours && min == alarm_min && is_alarm_set ==1){    //is_alarm_set==1 is to make sure alarm does not go off at 12:00
				TCCR0A |= (1<<WGM01);
				TIMSK0 |=(1<<OCIE0A);    //Timer for alarm sound interrupt enabled
				OCR0A = 30;
				TCCR0B |= (1<<CS00) | (1<<CS01);
				is_alarm_set++;  //otherwise this gets stuck in a weird loop;			
	}
	return;
	// when return to main, main will be busy cycling through seconds to count the next minute, so, display need to reset display to new value of the time being set
}



ISR(INT0_vect)    //set alarm
{
	blank_display();
	alarm_hours = 0;
	alarm_min = 0;
	
	
	while ((PINC & (1<<PORTC3))) {    // While P3 is High, goes low on button press
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			if (alarm_hours == 11){
				alarm_hours=-1;    //New Stuff Alarm hour set smooth transition from 11,12,1
			}
			
			alarm_hours++;
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if (alarm_hours == 0){
				alarm_hours = 12;
				//alarm_hours = 11;    //New Stuff
			}
			alarm_hours--;
			_delay_ms(250);
		}
		if (alarm_hours == 12){    //New Stuff
			alarm_hours = 1;
		}

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

		//if (alarm_hours == 13){
			//alarm_hours = 1;
		//}
		
		
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
		is_alarm_set =1;
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
	alarm_hours = -1;
	alarm_min = -1;
	is_alarm_set =0;
	return;
	
}


ISR(PCINT2_vect)    //Interrupt to call set_time() on PIN2 (PD0)
{
	blank_display();
	_delay_ms(1000);
	set_time();
	PCIFR |= (1<<PCIF2);    // maybe get rid of double interupt
	return;
}
