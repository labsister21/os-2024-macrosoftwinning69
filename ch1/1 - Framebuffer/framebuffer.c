#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

// Deklarasi Konstanta
uint16_t* const framebuffer = (uint16_t*)FRAMEBUFFER_MEMORY_OFFSET;
uint8_t cursor_row = 0;
uint8_t cursor_col = 0;

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    uint8_t cursor_row = 0;
    uint8_t cursor_col = 0;

    // Atur byte untuk low cursor
    port_byte_out(CURSOR_PORT_CMD, 0x0F);
    port_byte_out(CURSOR_PORT_DATA, (uint8_t)(cursor_location & 0xFF));
    // Atur byte untuk high cursor
    port_byte_out(CURSOR_PORT_CMD, 0x0E);
    port_byte_out(CURSOR_PORT_DATA, (uint8_t)((cursor_location >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    if (row >= FRAMEBUFFER_HEIGHT || col >= FRAMEBUFFER_WIDTH) {
        return; // Penanganan posisi yang invalid
    }

    uint8_t attribute = (bg << 4) | (fg & 0x0F);
    framebuffer[row * FRAMEBUFFER_WIDTH + col] = (uint16_t)attribute << 8 | c;
}

void framebuffer_clear(void) {
    // TODO : Implement
    for (size_t i = 0; i < FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT; ++i) {
        framebuffer[i] = (uint16_t)0x07 << 8; // Fill with empty character and default color
    }
}
