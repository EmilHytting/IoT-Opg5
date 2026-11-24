#include <assert.h>
#include <stdio.h>
#include "../Source/UART/UART.c"

volatile uint8_t UCSR0A, UCSR0B, UCSR0C, UCSR1A, UCSR1B, UCSR1C, UDR0, UDR1;
volatile uint16_t UBRR0, UBRR1;

static void receive(uint8_t byte) {
    UDR0 = byte;
    USART0_RX_vect();
}

static void start(void) {
    RS232Init();
    receive(0);
    receive(';');
    assert(state == WAIT_COLON && UART_Queued() == 0);
    receive(':');
    for (unsigned i = 0; name[i]; i++) {
        assert(UCSR1B & (1 << UDRIE1));
        USART1_UDRE_vect();
        assert(UDR1 == (uint8_t)name[i]);
    }
    assert(state == WAIT_START);
    assert(!(UCSR1B & (1 << UDRIE1)));
    receive(99);
    assert(state == WAIT_START);
    receive(';');
    assert(state == DATA);
}

int main(void) {
    start();
    /* Four wraps, including all control-like bytes, with bursty draining. */
    for (unsigned base = 0; base < 1024; base += 64) {
        for (unsigned i = 0; i < 64; i++) receive((uint8_t)(base + i));
        assert(UART_Queued() == 64);
        for (unsigned i = 0; i < 64; i++) {
            USART1_UDRE_vect();
            assert(UDR1 == (uint8_t)(base + i));
        }
        assert(UART_Queued() == 0 && state == DATA);
    }
    UART_Stats stats;
    UART_GetStats(&stats);
    for (unsigned i = 0; i < 5; i++) assert(stats.last[i] == 251 + i);
    receive('_'); /* Expected 0, so this is control. */
    assert(state == WAIT_COLON && UART_Queued() == 0);
    assert(!(UCSR1B & (1 << UDRIE1)));

    start();
    for (unsigned i = 0; i < 255; i++) receive((uint8_t)i);
    assert(UART_Queued() == 255 && state == DATA);
    receive(255);
    assert(state == FAULT && UART_Queued() == 0);
    UART_Service();
    const char *wanted = ";255;";
    for (unsigned i = 0; wanted[i]; i++) {
        USART1_UDRE_vect();
        assert(UDR1 == (uint8_t)wanted[i]);
    }
    assert(report_pending == 0 && !(UCSR1B & (1 << UDRIE1)));
    /* An interrupt during report formatting must not transmit stale text. */
    report_pending = 3;
    UDR1 = 123;
    UCSR1B |= (1 << UDRIE1);
    USART1_UDRE_vect();
    assert(UDR1 == 123 && !(UCSR1B & (1 << UDRIE1)));
    receive('-');
    assert(state == WAIT_COLON);

    start();
    UCSR0A = (1 << DOR0);
    receive(0);
    assert(state == FAULT);
    UCSR0A = 0;
    receive('_');
    assert(state == WAIT_COLON);
    UART_Service();
    assert(!(UCSR1B & (1 << UDRIE1)));
    puts("PASS: handshake, 0-255, wraps, last five, reset, full buffer, overrun report, hardware error");
    return 0;
}
