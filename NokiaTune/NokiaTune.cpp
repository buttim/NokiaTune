#include "NokiaTune.h"
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "pitches.h"
extern "C" {
#include "irmp.h"
#include "serial.h"
};

notes melody[]={
	//mi-re-fa#-sol#-do#-si-re-mi-si-la-do#-mi-la
	E6,D6,FS5,GS5,
	CS6,B5,D5,E5,	
	B5,A5,CS5,E5,
	A5
  /* scala diatonica
  	G6,F6,A5,B5,
	E6,D6,F5,G5,
	D6,C6,E5,G5, 
	C6*/
};

int duration[]={
	1,1,2,2,
	1,1,2,2,
	1,1,2,2,
	6
};

IRMP_DATA irmp_data;
bool playing=false;
int deviceId=0,tempo,nota,timeToWait,t=0,ripetizioni;

static void SetFreq(unsigned freq) {
	if (freq==0) {
		OCR1C=0;
		return;
	}

	for (unsigned int i=0;i<16;i++)
		if (freq>=F_CPU/(2UL*(1<<i)*256)) {
			OCR1C=F_CPU/(2UL*(1<<i)*freq)-1;
			TCCR1=_BV(CTC1)|(1+i);
			return;
		}
	TCCR1&=~(_BV(CS10)|_BV(CS11)|_BV(CS12)|_BV(CS13));
	OCR1C=0;
}

void timer0_init(void) {
	OCR0A=(F_CPU / F_INTERRUPTS / 8) - 1;
	TCCR0B|=_BV(CS01);
	TCCR0A|=_BV(WGM01);
	TIMSK|=_BV(OCIE0A);
}

void timer1_init() {
	TCCR1 |= _BV(CTC1);
	OCR1C=255;
	OCR1B=128;
	GTCCR|=_BV(COM1B0)|_BV(PWM1B);
}

void StopPlay() {
	playing=false;
	SetFreq(0);
}

ISR(TIMER0_COMPA_vect) {
	irmp_ISR();
	if (!playing) return;
	if (++t==15) {
		t=0;
		if (--timeToWait==0)
			if (++nota==sizeof melody/sizeof *melody) {
				if (++ripetizioni==3)
					StopPlay();
				else {
					nota=-1;
					timeToWait=2000;
					SetFreq(0);
				}
			}				
			else {
				SetFreq(melody[nota]);
				timeToWait=10*tempo*duration[nota];
			}
	}
}

void StartPlay(int id) {
	switch (id) {
	case 0:	//giusto
		tempo=15;
		melody[sizeof melody/sizeof *melody-1]=A5;
		break;
	case 1:	//stecca in basso
		//case 0xFF:
		tempo=15;
		melody[sizeof melody/sizeof *melody-1]=C5;
		break;
	case 2:	//stecca in alto (veloce)
		tempo=12;
		melody[sizeof melody/sizeof *melody-1]=D6;
		break;
	case 3:	//manca ultima nota (lento)
		tempo=20;
		melody[sizeof melody/sizeof *melody-1]=(notes)0;
		break;
	}
	SetFreq(melody[nota=0]);
	timeToWait=10*tempo*duration[0];
	ripetizioni=t=0;
	playing=true;
}

int main(void) {
	DDRB|=_BV(DDB3)|_BV(DDB4);
#ifdef DEBUG
	DDRB|=_BV(DDB0);	//seriale
#endif
	PORTB|=_BV(PB1);	//pullup per ponticello configurazione
	
	timer0_init();
	timer1_init();
	irmp_init();                                                            // initialize irmp
	sei();
	

	if ((PINB&_BV(PB0))==0)
		deviceId=0;
	else
		deviceId=1;

	for (int i=0;i<=deviceId;i++) {
		SetFreq(220);
		_delay_ms(40);
		SetFreq(0);
		_delay_ms(80);
	}	
	
	while(1) {
        if (irmp_get_data (&irmp_data)) {
#ifdef DEBUG
			char s[128];
			sprintf(s,"proto: %d addr: %d cmd: %d flags: %d\r\n",irmp_data.protocol,irmp_data.address,irmp_data.command,irmp_data.flags);
			UART_TX_STRING(s);
#endif
			if (irmp_data.flags==0) {
				if ((deviceId==0 && (irmp_data.command==69 || irmp_data.command==71)) ||
						(deviceId==1 && (irmp_data.command==70 || irmp_data.command==68))) {
					StartPlay(irmp_data.command-68);
				}
			else if (irmp_data.flags==0 && irmp_data.command==67) 
				StopPlay();
			}				
		}			
	}
}