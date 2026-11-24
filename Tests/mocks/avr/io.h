#pragma once
#include <stdint.h>
extern volatile uint8_t UCSR0A, UCSR0B, UCSR0C, UCSR1A, UCSR1B, UCSR1C, UDR0, UDR1;
extern volatile uint16_t UBRR0, UBRR1;
#define RXEN0 4
#define RXCIE0 7
#define UCSZ01 2
#define UCSZ00 1
#define TXEN1 3
#define UDRIE1 5
#define UCSZ11 2
#define UCSZ10 1
#define DOR0 3
#define FE0 4
#define UPE0 2
