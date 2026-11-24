#include "../../Include/UART/UART.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdio.h>

enum { WAIT_COLON, SEND_NAME, WAIT_START, DATA, FAULT };
static volatile uint8_t buffer[256];
/* One empty slot distinguishes full/empty. uint8_t wraps at 256. */
static volatile uint8_t head, tail, state, expected;
static volatile uint8_t last[5], high_water, lost, report_pending;
static volatile uint32_t overruns;
static volatile uint8_t name_index, report_index;
static char report[7];
static const char name[] = ":Lonneberg:";

static void Reset(void) {
    UCSR1B &= ~(1 << UDRIE1);
    state = WAIT_COLON;
    head = tail = expected = 0;
    report_pending = 0;
    name_index = report_index = 0;
}

void RS232Init(void) {
    UBRR0 = MyUBRR;
    UCSR0A = 0;
    UCSR0B = (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UBRR1 = MyUBRR;
    UCSR1A = 0;
    UCSR1B = (1 << TXEN1);
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
    Reset();
}

ISR(USART0_RX_vect) {
    uint8_t status = UCSR0A;
    uint8_t byte = UDR0;
    if (status & ((1 << DOR0) | (1 << FE0) | (1 << UPE0))) {
        if (state == DATA) {
            lost = expected;
            overruns++;
            report_pending = 1;
            report_index = 0;
            state = FAULT;
            tail = head;
        }
        return;
    }
    if (state == WAIT_COLON) {
        if (byte == ':') {
            name_index = 0;
            state = SEND_NAME;
            UCSR1B |= (1 << UDRIE1);
        }
        return;
    }
    if (state == WAIT_START) {
        if (byte == ';') {
            head = tail = expected = 0;
            for (uint8_t i = 0; i < 5; i++) last[i] = 0;
            state = DATA;
        } else if (byte == '_') Reset();
        return;
    }
    if (state == FAULT) {
        if (byte == '-' || byte == '_') Reset();
        return;
    }
    if (state != DATA) return;

    /* '_' (95) is binary data when expected, otherwise a reset.
       '-' (45) is control only in FAULT after an overrun. The protocol
       cannot distinguish a reset '_' from an expected data byte 95. */
    if (byte == '_' && byte != expected) { Reset(); return; }
    expected = (uint8_t)(byte + 1);
    uint8_t next = (uint8_t)(head + 1);
    if (next == tail) {
        lost = byte;
        overruns++;
        report_pending = 1;
        report_index = 0;
        state = FAULT;
        tail = head;
        return;
    }
    buffer[head] = byte;
    head = next;
    UCSR1B |= (1 << UDRIE1);
    for (uint8_t i = 0; i < 4; i++) last[i] = last[i + 1];
    last[4] = byte;
    uint8_t used = (uint8_t)(head - tail);
    if (used > high_water) high_water = used;
}

void UART_Service(void) {
    uint8_t make_report = 0, missing = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (state == FAULT && report_pending == 1) {
            missing = lost;
            report_pending = 3; /* Preparing; the TX ISR must not read yet. */
            make_report = 1;
        }
    }
    if (make_report) {
        snprintf(report, sizeof report, ";%u;", missing);
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (state == FAULT && report_pending == 3) report_pending = 2;
        }
    }
    /* Enable the short transmit ISR. It drains the ring even while
       the foreground is updating the display. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (state == FAULT && report_pending == 2) UCSR1B |= (1 << UDRIE1);
    }
}

ISR(USART1_UDRE_vect) {
    if (state == SEND_NAME) {
        UDR1 = name[name_index++];
        if (!name[name_index]) {
            state = WAIT_START;
            UCSR1B &= ~(1 << UDRIE1);
        }
    } else if (state == DATA && tail != head) {
        UDR1 = buffer[tail];
        tail = (uint8_t)(tail + 1);
        if (tail == head) UCSR1B &= ~(1 << UDRIE1);
    } else if (state == FAULT && report_pending == 2) {
        UDR1 = report[report_index++];
        if (!report[report_index]) {
            report_pending = 0;
            UCSR1B &= ~(1 << UDRIE1);
        }
    } else {
        UCSR1B &= ~(1 << UDRIE1);
    }
}

uint8_t UART_Queued(void) {
    uint8_t count;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { count = (uint8_t)(head - tail); }
    return count;
}

void UART_GetStats(UART_Stats *stats) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t i = 0; i < 5; i++) stats->last[i] = last[i];
        stats->high_water = high_water;
        stats->overruns = overruns;
        stats->state = state;
    }
}
