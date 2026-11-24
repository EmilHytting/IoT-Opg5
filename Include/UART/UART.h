#ifndef UART_H_
#define UART_H_
#include <stdint.h>
#include "../ProjectDefine/ProjectDefines.h"
#define BAUD 9600UL
#define MyUBRR ((F_CPU / (16UL * BAUD)) - 1UL)

typedef struct {
    uint8_t last[5]; /* Oldest to newest; empty positions are zero. */
    uint8_t high_water;
    uint8_t state;
    uint32_t overruns;
} UART_Stats;

void RS232Init(void);
void UART_Service(void);
uint8_t UART_Queued(void);
void UART_GetStats(UART_Stats *stats);
#endif
