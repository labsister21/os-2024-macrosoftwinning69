#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

// Deklarasi Konstanta
// uint16_t* const framebuffer = (uint16_t*)FRAMEBUFFER_MEMORY_OFFSET;
uint8_t cursor_row = 0;
uint8_t cursor_col = 0;

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    uint8_t cursor_location = r *FRAMEBUFFER_WIDTH + c;

    // Atur byte untuk low cursor
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, (uint8_t)(cursor_location & 0xFF));
    // Atur byte untuk high cursor
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (uint8_t)((cursor_location >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    if (row >= FRAMEBUFFER_HEIGHT || col >= FRAMEBUFFER_WIDTH) {
        return; // Penanganan posisi yang invalid
    }

    FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2] = c;
    FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2 + 1] = (bg << 4) | (fg & 0x0F);
}

void framebuffer_clear(void) {
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 80; j++) {
            framebuffer_write(i, j, 0x0, 0x7, 0x0);
        }
    }
}
