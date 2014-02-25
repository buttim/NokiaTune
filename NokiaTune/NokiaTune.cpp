#define F_CPU 8000000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
//#include <avr/power.h>
extern "C" {
	#include "irmp.h"
};
#include "pitches.h"

#define UART_TX_PORT PORTB
#define UART_TX_PIN PINB1

#define baud 9700
#define bit_delay (1000000/baud)

//write out a byte as software emulated Uart
void serialWrite(uint8_t bite){
	UART_TX_PORT&=~_BV(UART_TX_PIN);  //signal start bit
	_delay_us(bit_delay);

	for (uint8_t mask = 0x01; mask; mask <<= 1) {
		if (bite & mask) // choose bit
		UART_TX_PORT|=_BV(UART_TX_PIN);
		else
		UART_TX_PORT&=~_BV(UART_TX_PIN);
		_delay_us(bit_delay);
	}

	UART_TX_PORT|=_BV(UART_TX_PIN); //signal end bit
	_delay_us(bit_delay);
}

void UART_TX_STRING(char *s) {
	cli();
	while (*s) serialWrite((uint8_t)*(s++));
	sei();
}



void timer1_init (void) {
	#if defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)                // ATtiny45 / ATtiny85:

	#if F_CPU >= 16000000L
	OCR1C   =  (F_CPU / F_INTERRUPTS / 8) - 1;                              // compare value: 1/15000 of CPU frequency, presc = 8
	TCCR1   = (1 << CTC1) | (1 << CS12);                                    // switch CTC Mode on, set prescaler to 8
	#else
	OCR1C   =  (F_CPU / F_INTERRUPTS / 4) - 1;                              // compare value: 1/15000 of CPU frequency, presc = 4
	TCCR1   = (1 << CTC1) | (1 << CS11) | (1 << CS10);                      // switch CTC Mode on, set prescaler to 4
	#endif

	#else                                                                       // ATmegaXX:
	OCR1A   =  (F_CPU / F_INTERRUPTS) - 1;                                  // compare value: 1/15000 of CPU frequency
	TCCR1B  = (1 << WGM12) | (1 << CS10);                                   // switch CTC Mode on, set prescaler to 1
	#endif

	#ifdef TIMSK1
	TIMSK1  = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
	#else
	TIMSK   = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
	#endif
}

#ifdef TIM1_COMPA_vect                                                      // ATtiny84
#define COMPA_VECT  TIM1_COMPA_vect
#else
#define COMPA_VECT  TIMER1_COMPA_vect                                       // ATmega
#endif

ISR(COMPA_VECT)                                                             // Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
	(void) irmp_ISR();                                                        // call irmp ISR
	// call other timer interrupt routines...
}

static void SetFreq(unsigned freq) {
	if (freq==0) {
		timer1_init();
		return;
	}

	TCCR1 |= _BV(CTC1);
	OCR1C=255;
	OCR1B=128;
	GTCCR|=_BV(COM1B0)|_BV(PWM1B);
	
	for (unsigned int i=0;i<16;i++)
		if (freq>=F_CPU/(2UL*(1<<i)*256)) {
			OCR1C=F_CPU/(2UL*(1<<i)*freq)-1;
			TCCR1=_BV(CTC1)|(1+i);
			return;
		}
	TCCR1&=~(_BV(CS10)|_BV(CS11)|_BV(CS12)|_BV(CS13));
	OCR1C=0;
}

notes melody[] = {
	//mi-re-fa#-sol#-do#-si-re-mi-si-la-do#-mi-la
	E6,D6,FS5,GS5,
	CS6,B5,D5,E5,	
	B5,A5,CS5,E5,
	A5
  /*	G6,F6,A5,B5,
	E6,D6,F5,G5,
	D6,C6,E5,G5, 
	C6*/
};

int duration[]= {
	1,1,2,2,
	1,1,2,2,
	1,1,2,2,
	6
};

IRMP_DATA irmp_data;
int tempo;

void PlayTune(int id) {
	switch (id) {
	case 0:	//giusto
		tempo=9;
		melody[sizeof melody/sizeof *melody-1]=A5;
		break;
	case 1:	//stecca in basso
	//case 0xFF:
		tempo=9;
		melody[sizeof melody/sizeof *melody-1]=A3;
		break;
	case 2:	//stecca in alto (veloce)
		tempo=8;
		melody[sizeof melody/sizeof *melody-1]=A6;
		break;
	case 3:	//manca ultima nota (lento)
		tempo=10;
		melody[sizeof melody/sizeof *melody-1]=(notes)0;
		break;
	}

	int i,j;
	for (i=0;i<sizeof melody/sizeof *melody;i++) {
		SetFreq(melody[i]);
		for (j=0;j<duration[i]*tempo;j++)
		_delay_ms(10);
		SetFreq(0);
		for (j=0;j<tempo/2;j++)
		_delay_ms(10);
	}
	SetFreq(0);
}

int main(void) {
	//uint8_t id=eeprom_read_byte((uint8_t*)0);
		
	DDRB|=_BV(DDB3)|_BV(DDB4)|_BV(DDB1);
	
	irmp_init();                                                            // initialize irmp
	timer1_init();
	sei();
	
	SetFreq(220);
	_delay_ms(40);
	SetFreq(0);
	
	while(1) {
        if (irmp_get_data (&irmp_data)) {
#ifdef DEBUG
			char s[128];
			sprintf(s,"proto: %d addr: %d cmd: %d flags: %d\r\n",irmp_data.protocol,irmp_data.address,irmp_data.command,irmp_data.flags);
			UART_TX_STRING(s);
#endif
			if (irmp_data.flags==0 && irmp_data.command>=68 && irmp_data.command<=71)
				PlayTune(irmp_data.command-68);
		}			
	}

	/*_delay_ms(500);
	while (1) { 
		PlayTune();
		_delay_ms(3000);
	}*/
}