#include "Include/ProjectDefine/ProjectDefines.h"
#include "Include/UART/UART.h"
#include "Include/Timer/Timer.h"
#include "Include/Display/ssd1306_328pb.h"
#include <util/atomic.h>
#include <stdio.h>

static volatile uint32_t falling_edges, rising_edges;
static volatile uint8_t button_dirty;

ISR(INT0_vect) {
    /* Any change on PD2: low = falling, high = rising. */
    if (PIND & (1 << PD2)) rising_edges++;
    else falling_edges++;
    PORTB ^= (1 << YellowLedBit);
    button_dirty = 1;
}

static void Button_Init(void) {
    DDRD &= ~(1 << DDD2);
    PORTD |= (1 << PORTD2); /* Button from PD2 to GND. */
    EICRA = (EICRA & ~((1 << ISC01) | (1 << ISC00))) | (1 << ISC00);
    EIFR = (1 << INTF0);
    EIMSK |= (1 << INT0);
}

static void Display_Line(uint8_t line) {
    char text[22]; /* 128 / 6 = 21 characters per line. */
    uint32_t falling, rising, seconds;
    UART_Stats stats;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        falling = falling_edges;
        rising = rising_edges;
        seconds = timer_seconds;
    }
    UART_GetStats(&stats);
    switch (line) {
    case 0: snprintf(text, sizeof text, "Ned: %lu", (unsigned long)falling); break;
    case 1: snprintf(text, sizeof text, "Op:  %lu", (unsigned long)rising); break;
    case 2: snprintf(text, sizeof text, "Sek: %lu", (unsigned long)seconds); break;
    case 3:
        snprintf(text, sizeof text, "%03u %03u %03u %03u %03u",
                 stats.last[0], stats.last[1], stats.last[2], stats.last[3], stats.last[4]);
        break;
    case 4: snprintf(text, sizeof text, "Embedded Styrer !!!"); break;
    case 5: snprintf(text, sizeof text, "Buffer max: %u/255", stats.high_water); break;
    case 6: snprintf(text, sizeof text, "Tab: %lu", (unsigned long)stats.overruns); break;
    default: snprintf(text, sizeof text, "Lonneberg state: %u", stats.state); break;
    }
    ssd1306_write_line(line, text);
}

int main(void) {
    uint8_t display_ok, pending_lines = 0xff;
    DDRB |= (1 << GreenLedBit) | (1 << YellowLedBit);
    /* A missing display must not block the UART test. */
    display_ok = (ssd1306_init() == 0);
    if (display_ok) ssd1306_clear();
    RS232Init();
    Button_Init();
    Timer1_Init_ms(1000);
    sei();
    for (;;) {
        UART_Service(); /* Prepare reports; byte transfer uses short ISRs. */
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (timer_second_flag) {
                timer_second_flag = 0;
                pending_lines = 0xff;
            }
            if (button_dirty) {
                button_dirty = 0;
                pending_lines |= 3;
            }
        }
        if (display_ok && pending_lines) {
            /* Service UART between each display line. */
            uint8_t line = 0;
            while (!(pending_lines & (1 << line))) line++;
            pending_lines &= ~(1 << line);
            Display_Line(line);
        }
    }
}
