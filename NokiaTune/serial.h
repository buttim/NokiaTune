#define UART_TX_PORT PORTB
#define UART_TX_PIN PINB1

#define baud 9700
#define bit_delay (1000000/baud)

//write out a byte as software emulated Uart
void UART_TX_STRING(char *s);

