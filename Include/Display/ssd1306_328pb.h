#pragma once


#include <stdint.h>
#include <stdbool.h>

#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64  // Skift til 32 for 128x32 displays
#define SSD1306_I2C_ADDR        0x3C

typedef enum {
	SSD1306_COLOR_BLACK = 0x00,
	SSD1306_COLOR_WHITE = 0x01
} ssd1306_color_t;

// Funktioner til ATmega328PB Direct Mode
int  ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_pixel(uint8_t x, uint8_t y, ssd1306_color_t color);
void ssd1306_draw_line(int x0, int y0, int x1, int y1, ssd1306_color_t color);
void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1306_color_t color);
void ssd1306_set_contrast(uint8_t contrast);
void ssd1306_display_on(bool on);
void ssd1306_clear_line(uint8_t line);
void ssd1306_write_line(uint8_t line, const char *text);