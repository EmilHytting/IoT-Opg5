/*
 * ssd1306_328pb.c
 *
 * Created: 17-11-2026 15:02:56
 *  Author: ltpe
 */

#include "../../Include/ProjectDefine/ProjectDefines.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdlib.h>
#include "../../Include/Display/ssd1306_328pb.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define SCL_CLOCK 100000L  // 400 kHz I2C

#define SSD1306_CTRL_CMD        0x00
#define SSD1306_CTRL_DATA       0x40

// Minimal 5x7 ASCII font (understøtter standard bogstaver og tal)
static const uint8_t font5x7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // 35 #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // 41 )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 48 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // 64 @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 65 A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 68 D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 71 G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 72 H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 74 J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 77 M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 78 N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 79 O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 81 Q
    {0x7f, 0x09, 0x09, 0x19, 0x66}, // 82 R
    {0x26, 0x49, 0x49, 0x49, 0x32}, // 83 S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 84 T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 85 U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 86 V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // 90 Z
};



// --- Interne TWI0 funktioner specielt til ATmega328PB ---

static void twi_init(void) {
	TWCR0 = 0; // Sluk TWI0 fuldstændigt og nulstil state-machine
	_delay_us(10);
	// uint16_t counter = 0;
	// while (counter++ < 5000);

	PRR0 &= ~(1 << PRTWI0); // Sikr at TWI0 ikke er slukket

	DDRC  &= ~((1 << PC4) | (1 << PC5)); // Sæt PC4 og PC5 til input
    PORTC |=  ((1 << PC4) | (1 << PC5)); // Aktivér interne pull-up modstande
	TWSR0 = 0x00;           // Nulstil prescaler
	TWBR0 = (uint8_t)(((F_CPU / SCL_CLOCK) - 16) / 2);
}

static int twi_start(uint8_t address) {
	TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	// Timeout-sikring så koden ikke hænger evigt, hvis bussen fejler
	uint16_t timeout = 0;
	while (!(TWCR0 & (1 << TWINT))) {
		timeout++;
		if (timeout > 20000) return -1;
	}

	uint8_t status = TWSR0 & 0xF8;
	if (status != 0x08 && status != 0x10) return -1;

	TWDR0 = address;
	TWCR0 = (1 << TWINT) | (1 << TWEN);

	timeout = 0;
	while (!(TWCR0 & (1 << TWINT))) {
		timeout++;
		if (timeout > 20000)
		{
			return -1;
		}
	}

	if ((TWSR0 & 0xF8) != TW_MT_SLA_ACK) return -1;
	return 0;
}

static int twi_write_byte(uint8_t data) {
	TWDR0 = data;
	TWCR0 = (1 << TWINT) | (1 << TWEN);

	uint16_t timeout = 0;
	while (!(TWCR0 & (1 << TWINT))) {
		if (++timeout > 20000) return -1;
	}

	if ((TWSR0 & 0xF8) != TW_MT_DATA_ACK) return -1;
	return 0;
}

static void twi_stop(void) {
	TWCR0 = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	//while (TWCR0 & (1 << TWSTO)); // LTPE
}

static int i2c_write_packet(uint8_t reg_addr, const uint8_t *data, size_t len) {
	if (twi_start((SSD1306_I2C_ADDR << 1) | TW_WRITE) != 0) {
		twi_stop();
		return -1;
	}

	if (twi_write_byte(reg_addr) != 0) {
		twi_stop();
		return -1;
	}

	for (size_t i = 0; i < len; i++) {
		if (twi_write_byte(data[i]) != 0) {
			twi_stop();
			return -1;
		}
	}

	twi_stop();
	return 0;
}

static int ssd1306_write_cmd(uint8_t cmd) {
	return i2c_write_packet(SSD1306_CTRL_CMD, &cmd, 1);
}

// --- SSD1306 Driver Funktioner ---

int ssd1306_init(void) {
	twi_init();
	_delay_ms(50);

	int err = 0;
	err |= ssd1306_write_cmd(0xAE); // Display off
	err |= ssd1306_write_cmd(0xD5); // Set display clock div
	err |= ssd1306_write_cmd(0x80);

	err |= ssd1306_write_cmd(0xA8); // Set multiplex
	err |= ssd1306_write_cmd(SSD1306_HEIGHT - 1);

	err |= ssd1306_write_cmd(0xD3); // Set display offset
	err |= ssd1306_write_cmd(0x00);

	err |= ssd1306_write_cmd(0x40); // Set start line
	err |= ssd1306_write_cmd(0x8D); // Charge pump
	err |= ssd1306_write_cmd(0x14);

	err |= ssd1306_write_cmd(0x20); // Memory mode (Page mode)
	//err |= ssd1306_write_cmd(0x02);
	// LTPE ændret linjen herovertil linjen herunder for at sætte til Page mode
	err |= ssd1306_write_cmd(0x00);

	err |= ssd1306_write_cmd(0xA1); // Segment remap
	err |= ssd1306_write_cmd(0xC8); // Com scan dec

	err |= ssd1306_write_cmd(0xDA); // Set com pins
	#if (SSD1306_HEIGHT == 64)
	err |= ssd1306_write_cmd(0x12);
	#elif (SSD1306_HEIGHT == 32)
	err |= ssd1306_write_cmd(0x02);
	#endif

	err |= ssd1306_write_cmd(0x81); // Contrast
	err |= ssd1306_write_cmd(0xCF);

	err |= ssd1306_write_cmd(0xD9); // Pre-charge
	err |= ssd1306_write_cmd(0xF1);

	err |= ssd1306_write_cmd(0xDB); // VCOM detect
	err |= ssd1306_write_cmd(0x40);

	err |= ssd1306_write_cmd(0xA4); // Resume to RAM content
	err |= ssd1306_write_cmd(0xA6); // Normal display
	err |= ssd1306_write_cmd(0xAF); // Display on

	return err;
}

void ssd1306_clear(void) {
	uint8_t zero_buffer[128] = {0};
	for (uint8_t page = 0; page < (SSD1306_HEIGHT / 8); page++) {
		ssd1306_write_cmd(0xB0 + page);
		ssd1306_write_cmd(0x00);
		ssd1306_write_cmd(0x10);
		i2c_write_packet(SSD1306_CTRL_DATA, zero_buffer, 128);
	}
}

void ssd1306_draw_pixel(uint8_t x, uint8_t y, ssd1306_color_t color) {
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

	uint8_t page = y / 8;
	uint8_t bit = y % 8;

	ssd1306_write_cmd(0xB0 + page);
	ssd1306_write_cmd(0x00 + (x & 0x0F));
	ssd1306_write_cmd(0x10 + ((x >> 4) & 0x0F));

	uint8_t data = (color == SSD1306_COLOR_WHITE) ? (1 << bit) : 0x00;
	i2c_write_packet(SSD1306_CTRL_DATA, &data, 1);
}

void ssd1306_draw_line(int x0, int y0, int x1, int y1, ssd1306_color_t color) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;

	while (1) {
		ssd1306_draw_pixel((uint8_t)x0, (uint8_t)y0, color);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1306_color_t color) {
	ssd1306_draw_line(x, y, x + w - 1, y, color);
	ssd1306_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
	ssd1306_draw_line(x, y, x, y + h - 1, color);
	ssd1306_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void ssd1306_set_contrast(uint8_t contrast) {
	ssd1306_write_cmd(0x81);
	ssd1306_write_cmd(contrast);
}

void ssd1306_display_on(bool on) {
	ssd1306_write_cmd(on ? 0xAF : 0xAE);
}

// Slet en bestemt linje (line 0 til 3) ved at skrive 128 nuller over dens 2 sider
// void ssd1306_clear_line(uint8_t line) {
//     if (line > 3) return;
//     uint8_t start_page = line * 2;
//     uint8_t blank_data[128] = {0};

//     for (uint8_t p = start_page; p < start_page + 2; p++) {
//         ssd1306_write_cmd(0xB0 + p); // Sæt page
//         ssd1306_write_cmd(0x00);     // Low column
//         ssd1306_write_cmd(0x10);     // High column
//         // Send 128 tomme bytes til hele rækken
//         // (Brug din eksisterende i2c_write_packet funktion)
//         i2c_write_packet(SSD1306_CTRL_DATA, blank_data, 128);
//     }
// }

// // Skriv tekst på en specifik linje (0 til 3)
// void ssd1306_write_line(uint8_t line, const char *text) {
//     if (line > 3) return;

//     // Først rydder vi linjen, så der ikke hænger gamle tegn rester
//     ssd1306_clear_line(line);

//     uint8_t start_page = line * 2;

//     // Vi skriver tekst over 2 sider (øverste og nederste halvdel af bogstaverne)
//     for (uint8_t p_offset = 0; p_offset < 2; p_offset++) {
//         ssd1306_write_cmd(0xB0 + (start_page + p_offset));
//         ssd1306_write_cmd(0x00);
//         ssd1306_write_cmd(0x10);

//         if (twi_start((SSD1306_I2C_ADDR << 1) | 0) == 0) {
//             twi_write_byte(SSD1306_CTRL_DATA);

//             const char *ptr = text;
//             while (*ptr) {
//                 char c = *ptr++;
//                 if (c >= 'a' && c <= 'z') c -= 32; // Lav små bogstaver til store

//                 uint8_t char_index = 0;
//                 if (c >= 32 && c <= 90) {
//                     char_index = c - 32;
//                 }

//                 // Send 5 bytes for selve tegnet
//                 for (uint8_t i = 0; i < 5; i++) {
//                     //uint8_t column_data = font5x7[char_index][i];
// 					uint8_t column_data = pgm_read_byte(&(font5x7[char_index][i]));
//                     // Hvis vi er på nederste side (p_offset == 1), skifter vi data-bitene ned
//                     if (p_offset == 1) {
//                         column_data >>= 4;
//                     } else {
//                         column_data &= 0x0F;
//                     }
//                     twi_write_byte(column_data);
//                 }
//                 // 1 tom kolonne mellem bogstaver
//                 twi_write_byte(0x00);
//             }
//             twi_stop();
//         }
//     }
// }

// Slet én enkelt linje (0 til 7, da hver linje nu fylder 1 page / 8 pixels)
void ssd1306_clear_line(uint8_t line) {
    if (line > 7) return;

    ssd1306_write_cmd(0xB0 + line); // Sæt page (0-7)
    ssd1306_write_cmd(0x00);         // Low column
    ssd1306_write_cmd(0x10);         // High column

    uint8_t blank_data[128] = {0};
    i2c_write_packet(SSD1306_CTRL_DATA, blank_data, 128);
}

// Skriv tekst på en specifik linje (0 til 7) med 5x7 font
void ssd1306_write_line(uint8_t line, const char *text) {
    if (line > 7) return;

    // Ryd linjen først
    ssd1306_clear_line(line);

    // Sæt position for den pågældende page
    ssd1306_write_cmd(0xB0 + line);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0x10);

    if (twi_start((SSD1306_I2C_ADDR << 1) | 0) == 0) {
        twi_write_byte(SSD1306_CTRL_DATA);

        const char *ptr = text;
        uint8_t remaining = SSD1306_WIDTH / 6;
        while (*ptr && remaining--) {
            char c = *ptr++;
            if (c >= 'a' && c <= 'z') c -= 32; // Små bogstaver til store

            uint8_t char_index = 0;
            if (c >= 32 && c <= 90) {
                char_index = c - 32;
            }

            // Send de 5 kolonner for tegnet direkte uden bit-shifting
            for (uint8_t i = 0; i < 5; i++) {
                uint8_t column_data = pgm_read_byte(&(font5x7[char_index][i]));
                twi_write_byte(column_data);
            }
            // 1 tom kolonne mellem tegn
            twi_write_byte(0x00);
        }
        twi_stop();
    }
}
