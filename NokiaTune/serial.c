#include "NokiaTune.h"
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "serial.h"

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
