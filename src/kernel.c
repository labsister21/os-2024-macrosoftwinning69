#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
// #include "header/driver/keyboard.h"
#include "header/background.h"

void write_string(int row, int col, char* str, int fg, int bg) {
    while (*str != '\0') {
        framebuffer_write(row, col, *str, fg, bg);
        col++;
        str++;
    }
}

void write_bg(int i, int j, int color) {
    framebuffer_write(i, j, 0x0, 0x0, color);
} 

void create_bg() {
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 80; j++) {
            if (i < 5) {
               write_bg(i, j, bg[i][j]);
            } else if (i < 23) {
                write_bg(i, j, bg[i][j]);
            } else {
                write_bg(i, j, bg[i][j]);
            }
        }
    }
}

void kernel_setup(void) {
    // uint32_t a;
    // uint32_t volatile b = 0x0000BABE;
    // __asm__("mov $0xCAFE0000, %0" : "=r"(a));

    // Load GDT
    load_gdt(&_gdt_gdtr);

    // Initialize IDT and keyboard
    activate_keyboard_interrupt();

    // Framebuffer operations
    framebuffer_clear();
    create_bg();
    write_string(10, 8, "Hello, User!", 0, 0x2);
    write_string(11, 11, "Welcome to Macrosoft Winning OS!", 0, 0x2);
    framebuffer_set_cursor(0, 0);
    // __asm__("int $0x4");

    while (true);
    // int col = 0;
    // keyboard_state_activate();
    // while (true){
    //     char c;
    //     get_keyboard_buffer(&c);
    //     if (c) framebuffer_write(0, col++, c, 0xF, 0);
    // }
}