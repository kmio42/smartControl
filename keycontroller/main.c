
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "twislave.h"
#include <avr/sleep.h>


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

//###################### Slave-Adresse
#define SLAVE_ADRESSE 0x50 								// Slave-Adresse

//###################### Macros
#define uniq(LOW,HEIGHT)	((HEIGHT << 8)|LOW)			// 2x 8Bit 	--> 16Bit
#define LOW_BYTE(x)        	(x & 0xff)					// 16Bit 	--> 8Bit
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)			// 16Bit 	--> 8Bit


#define sbi(ADDRESS,BIT) 	((ADDRESS) |= (1<<(BIT)))	// set Bit
#define cbi(ADDRESS,BIT) 	((ADDRESS) &= ~(1<<(BIT)))	// clear Bit
#define	toggle(ADDRESS,BIT)	((ADDRESS) ^= (1<<BIT))		// Bit umschalten

#define	bis(ADDRESS,BIT)	(ADDRESS & (1<<BIT))		// bit is set?
#define	bic(ADDRESS,BIT)	(!(ADDRESS & (1<<BIT)))		// bit is clear?


//######################################### PIN Mapping:
/*
## AVR-Board: Atmega88 auf Universalboard 0.9 (Karsten)
Folgende Pins stehen zur Verfügung:

* PC1 - Zeile 1/2
* PC2 - Zeile 3
* PC3 - Zeile 4

* PD0 - Spalte A - PCINT16
* PD1 - Spalte B - PCINT17
* PD2 - Spalte C - PCINT18
* PD3 - Spalte D - PCINT19
* PD4 - LED Reihe 1
* PD5 - LED Reihe 2
* PD6 - Lautsprecher in

* PB0 - Wakeup ESP?

### PIN-Matrix
1. Zeile 4
2. Zeile 1/2
3. Spalte D
4. Zeile 2
5. Spalte C
6. Röntgen1
7. Spalte B
8. Röntgen2
9. Spalte A
*/

//###################### Variablen

#define KEY_RPT 40
#define KEY_RPT_FIRST 100
#define KEY_DEB 8

enum KEYSTATE { KEY_RELEASED = 0b00, KEY_PRESSED = 0b01, KEY_REPEAT = 0b11};
struct keystate {
	unsigned int state:2; // 00: released, 01: pressed, 11: repeat
	unsigned int debounce: 6;
} key_state;

	uint16_t 	Variable=2345;				//Zähler
	uint16_t	buffer;
	uint16_t	low, hight;

struct key {
	unsigned int state:2;
	unsigned int debounce: 7;
	unsigned int row:2;
	unsigned int col:4;
	unsigned int debounce_released: 5;
} key_current;

volatile uint8_t leds;
volatile uint16_t clock = 0;
//################################################################################################### Helper Functions

void key_press_action() {
	txbuffer[1] = (key_current.state << 6) | ( key_current.row << 4) | key_current.col;
	PORTB &= ~(1 << PB4); //Set Interrupt for ESP
}

inline void key_reset_action() {
	txbuffer[1] = 0;
	PORTB |= (1 << PB4); //Release Interrupt for ESP
}

inline void i2c_read_addr(uint8_t address){
	if(address == 1)
		key_reset_action();
}
//################################################################################################### Interrupts

ISR ( PCINT2_vect) {
//
}

ISR (TIMER0_COMPA_vect) {
	static uint8_t row = 0;

	++clock;

	//get Inputs
	if ( row < 3 ) {
		uint8_t read_cols = (~PIND) & 0x0F; //readout Line

		uint8_t key = 0;

		if(row == 0)
			read_cols |= (~PIND & (1<<PD6));

		//determine Keypress from Row - high key dominates
		while (read_cols > 0) {
			key++;
			read_cols >>=1;
		}

		//Test for new key
		if(key && (key != key_current.col || row != key_current.row)) {
				//new key press
				key_current.col = key;
				key_current.row = row;
				key_current.debounce = KEY_DEB;
				key_current.state = KEY_RELEASED;
				key_current.debounce_released = KEY_DEB;
		}

		// Test for current key press
		if (row == key_current.row) { // if current key is from same line
			if ( key && (key == key_current.col) ) { //continuously pressed
				key_current.debounce_released = KEY_DEB;
				key_current.debounce --;
				switch(key_current.state) {
				case KEY_RELEASED:
					if(key_current.debounce == 0) {
						key_current.state = KEY_PRESSED;
						key_current.debounce = KEY_RPT_FIRST;
						key_press_action();
						//0-1 KEYPRESS DETECTION
					}
					break;
				case KEY_PRESSED:
				case KEY_REPEAT:
					if(key_current.debounce == 0) {
						key_current.state = KEY_REPEAT;
						key_current.debounce = KEY_RPT;
						key_press_action();
						//KEY REPEAT DETECTION
					}
					break;
				}
			}
			else { //released
				key_current.debounce_released --;
				if(key_current.debounce_released == 0) {
					key_current.col = 0;
					key_current.state = KEY_RELEASED;
					key_current.row = 0;
				}
			}
		}
	}

	//set Outputs and Pullups

	if((++row) == 5)
		row = 0;

	// row select and pullups for key matrix
	if(row < 3) {
		//Output of Row
		DDRC = (0b10 << row);
		PORTC = (~(0b10 << row))&0x0E;
		//Pullups for Columns
		DDRD = 0x00;
		PORTD = 0x0F | (1 << PD6);
	}
	else if(row == 3) {
		PORTD = ((~(leds >> 4))& 0x0F) | (1 << PD4) | (1 << PD6);
		DDRD = 0x0F | (1 << PD4);
		DDRC = 0;
		PORTC = 0;
	}
	else if (row == 4) {
		PORTD = ((~leds)& 0x0F) | (1 << PD5) | (1 << PD6);
		DDRD = 0x0F | (1 << PD5);
		DDRC = 0;
		PORTC = 0;
	}
}


//################################################################################################### Initialisierung
void Initialisierung(void)
{
	cli();
	//### PORTS

	//### TWI
	init_twi_slave(SLAVE_ADRESSE);			//TWI als Slave mit Adresse slaveadr starten

	sei();
}

//################################################################################################### Hauptprogramm
int main(void)
{

	Initialisierung();
    //configure sleepmode
    set_sleep_mode(SLEEP_MODE_PWR_SAVE);

    //Matrix Timer Configuration
	TCCR0A = (1 << WGM01); //CTC Mode
	TCCR0B = (1 << CS01); // Presacler 8
	OCR0A = 128;	//upper limit
	TIMSK0 = (1 << OCIE0A); //Compare Match Interrupt

	//Pin Out
	DDRB = (1 << PB2) | (1 << PB4);
	PORTB |= (1 << PB2) |  (1 << PB4); //Enable ESP inverted, Interrupt ESP inverted

	sei();

	uint16_t next_sleep;

	while(1)
    {
		//PORTD ^= (1 << PD5);
		if(key_current.col) {
			next_sleep = clock + 1000;
			//leds = (key_current.state << 6) | ( key_current.row << 4) | key_current.col;
		}


		if((next_sleep - clock)&0x8000) {
			if(!(rxbuffer[0] & 0x01)) {
				PORTB |= (1 << PB2); //Disable ESP
			}
			leds = 0;
			TCCR0B = 0; //Stop Matrix Timer

			//Prepare for Wakeup
			DDRD = 0x00;
			PORTD = 0x0F | (1 << PD6);
			DDRC = 0xE;
			PORTC = 0;
			PCICR = (1<<PCIE2); // Enable Interrupts for Bank2  (PCINT 16 to 19)
			PCMSK2 = 0x4F; //activate PCINT 16 to 19 and 22
			sleep_mode();
			PCICR = 0; //disable further Interrupts
			PORTB &= ~(1 << PB2); //Enable ESP

			//Restart matrix function
			TCCR0B = (1 << CS01); //Restart Matrix Timer
			next_sleep = clock + 1000;
		}

		leds = rxbuffer[2];
		if(rxbuffer[0] & 0x02) {
			next_sleep = clock + 1000;
		}

	//############################
	} //end.while
} //end.main

