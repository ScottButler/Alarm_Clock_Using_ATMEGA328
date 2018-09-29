/* Alarm_Clock_Interrupt_Based */
// only time_value and maybe alarm value global
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
void set_time(int *hours, int *min);    // want this to be uint8_t arguments
void set_alarm(int *alarm_time_count, uint8_t *is_alarm_set);    // used to have (int alarm_time_count) as first parameter
void turn_off_alarm(uint8_t *is_alarm_set);


volatile int time_count;    // counts time in number of minuets
volatile uint8_t second_count;
volatile uint8_t OCR0A_going_down=1;    //Alarm sounds stuff, prob volatile



int main(void)
{
	// Data Direction
	DDRB |= (1<<DDB3) | (1<<DDB2) | (1<<DDB5);    //all output for PORTB except pb6 and pb7 for oscillator
	DDRC = 0x07;    // 0x07 = 0b00000111, PC0, PC1, PC2 = Indicators
	DDRD = 0xC0;    // 0xC0 = 0b11000000, DDD6 is 1 for OC0A output (alarm sound), NOW D7 is ' indicator

	// Port Initial Conditions
	PORTB |= (1<<PORTB2) | (1<<PORTB5);    //CS HIGH and CLK start HIGH

	// Crystal and Timer
	ASSR |= (1<<AS2);    //enable crystal as input for timer2
	TCCR2B |= (1<<CS22) | (1<<CS20);    //Pre-scale of 128
	TIMSK2 |= (1<<TOIE2);    // timer2 overflow interrupt enabled

	// Interrupts
	sei();
	
	// try with uint8_t later or uint16_t
	volatile int alarm_time_count;
	volatile int hours = 0;
	volatile int min = 0;

	volatile uint8_t is_alarm_set = 0;
	
	//  Initalize 7-Segment LED Driver
	send_command(0x09, 0x0F);    //Decode mode for digits 0-3 (0xFF is for all digits)
	send_command(0x0A, 0x00);    //Intensity bottom
	send_command(0x0B, 0x04);    //Scan digits (0,1,2,3,4),0,1 = min, 2,3 = hours digit4 = indicators
	send_command(0x0C, 0x01);    //Turn on display

	// Blank Screen then Initalize Values for hours and min
	blank_display();
	PORTC |= (1<<PORTC2);    // Turn On :
	
	set_time(&hours, &min);    // Initalize: time_value, hours, min and display
		
	while (1)
	{
		// min and hours might need to be int so that not overflow happens during math
		min = time_count%60;
		hours = ((time_count/60)%12);
		
		// set A.M./P.M. indicator LEDS
		if (time_count < 720) {
			PORTC |= (1<<PORTC0);    // Turn on A.M. LED
			PORTC &= ~(1<<PORTC1);    // Turn off P.M.
		}
		else {
			PORTC &= ~(1<<PORTC0);    // Turn OFF A.M. LED
			PORTC |= (1<<PORTC1);    // Turn ON P.M.
		}
		
		// check for alarm
		if ((time_count == alarm_time_count) && is_alarm_set == 1){
			TCCR0A |= (1<<WGM01);
			TIMSK0 |=(1<<OCIE0A);
			OCR0A = 30;
			TCCR0B |= (1<<CS00) | (1<<CS01);
			is_alarm_set++;  //otherwise this gets stuck in loop;
		}
		
		// button checks
		// Set Time
		if (!(PIND & (1<<PORTD0))) {
			blank_display();
			_delay_ms(1000);
			set_time(&hours, &min);
		}
		// Set Alarm
		if (!(PIND & (1<<PORTD2))) {
			set_alarm(&alarm_time_count, &is_alarm_set);
		}
		// Turn OFF Alarm
		if (!(PIND & (1<<PORTD3))) {
			turn_off_alarm(&is_alarm_set);
		}
		
				
		// Display hours
		if (hours==0 || hours==12){    //moved 2 digits
			//hours = 0;    // Might be able to delete
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		else if (hours<10){
			send_command(0x03,hours);
			send_command(0x04,0x0F);
		}
		else if (hours<12 && hours>9){
			send_command(0x03, hours%10);
			send_command(0x04,1);
		}
		// error check, hours will print 00
		if (hours > 12) {
			send_command(0x03, 0x00);
			send_command(0x04, 0x00);
		}
		
		// Display minutes
		send_command(0x01, min%10);    // sets min digits after every 60s
		send_command(0x02,min/10);
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
void set_time(int *hours, int *min)
{
	// stop interrupts here to stop time_count from increasing
	cli();
	_delay_ms(100);// maybe does something to let stuff clean up
	int hours_24_format = 0;
	time_count = 0;
	*hours = 0;
	*min = 0;
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
		
		*hours = (hours_24_format%12);
		
		if ((*hours)==0 || (*hours)==12){
			//hours = 0;    // keeps hours between 0 and 11
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		else if ((*hours)<10 && (*hours)!=0){
			send_command(0x03,*hours);
			send_command(0x04,0x0F);
		}
		else if ((*hours) == 10){
			send_command(0x03, 0x00);
			send_command(0x04,0x01);
		}
		else if ((*hours)==11){
			send_command(0x03, (*hours)%10);
			send_command(0x04,0x01);
		}
	}

	send_command(0x01,0x0F);    //Blank min briefly when set, blanking hours looked weird to me
	send_command(0x02,0x0F);

	_delay_ms(500);

	while ((PINC & (1<<PORTC3))){
		
		
		if (!(PINC & (1<<PORTC5))){    //press top button hours goes up
			(*min)++;
			_delay_ms(250);
		}
		if (!(PINC & (1<<PORTC4))){    //press bottom button hours goes down
			if ((*min) == 0){
				*min = 60;
			}
			(*min)--;
			_delay_ms(250);
		}
		if ((*min)<60){
			send_command(0x01, (*min)%10);
			send_command(0x02,(*min)/10);
		}
		
		else {
			send_command(0x01,0x00);    // while setting the time if increment min was held display would not show min at 00, would go from 59, pause then to 01
			send_command(0x02,0x00);
		}
		
		if ((*min) == 60){    // New Stuff, was(min == 61), now(min==60), i think when setting the time going upward 00 would be displayed twice
			*min = 0;
		}
		if ((*hours)==0 || (*hours)==12){    // Alarm bug fix beginning
			send_command(0x03, 0x02);
			send_command(0x04, 0x01);
		}
		else if ((*hours)<10 && (*hours)!=0){
			send_command(0x03,*hours);
			send_command(0x04,0x0F);

		}
		else if ((*hours) == 10){
			send_command(0x03,0x00);
			send_command(0x04,0x01);
		}
		else if ((*hours)==11){    // Last segment of alarm bug fix
			send_command(0x03, (*hours)%10);
			send_command(0x04,0x01);
		}    // End of alarm bug fix
		

	}
	time_count = (hours_24_format * 60);    // This was the same but it was: time_count = (hours * 60);
	time_count = time_count + *min;

	blank_display();
	_delay_ms(500);
	//re-set display to correct time

	// min
	send_command(0x01,(*min)%10);
	send_command(0x02,(*min)/10);
	// hours
	if ((*hours)==0 || (*hours)==12){
		//hours = 0;
		send_command(0x03, 2);
		send_command(0x04, 1);
	}
	else if ((*hours)<10){
		send_command(0x03,*hours);
		send_command(0x04,0x0F);
	}
	else if (*hours<12){
		send_command(0x03, (*hours)%10);
		send_command(0x04,1);
	}
	second_count = 0;
	_delay_ms(1000);
	sei();
	_delay_ms(100);    // test to see if alarm will work, or display glitch after set_time() stops
	return;
}



void set_alarm(int *alarm_time_count, uint8_t *is_alarm_set)    //set alarm, return type could be uint8_t and return 1 could be used for is alarm_set, same for turn off alarm
{
	cli();    // i think this will allow alarm to work
	_delay_ms(100);
	blank_display();
	PORTD |= (1<<PORTD7);    // Turn on Alarm Indicator
	
	int alarm_hours_24 = 0;    // should be uint8_t if everything starts working
	uint8_t alarm_hours = 0;
	uint8_t alarm_min = 0;
	*alarm_time_count = 0;
	
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

	/*
	if (is_time_set_yet==0){    // ?????
		blank_display();
	}
	*/
	// might want to do math with a local variable
	*alarm_time_count = (alarm_hours_24 * 60);
	*alarm_time_count = *alarm_time_count + alarm_min;
	*is_alarm_set = 1;
	sei();
	_delay_ms(100);
	return;
}

void turn_off_alarm(uint8_t *is_alarm_set)    //Turn off all alarm stuff
{
	TCCR0B = 0x00;    // Turns off Speaker
	PORTD &= ~(1<<PORTD7);    // Turn OFF Alarm Indicator
	PORTD &= ~(1<<PORTD6);    // Turn OFF speaker led LED
	*is_alarm_set = 0;
	return;	
}


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

// Alarm sound
ISR (TIMER0_COMPA_vect)
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
