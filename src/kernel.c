#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
// #include "header/driver/keyboard.h"


void kernel_setup(void) {
    uint32_t a;
    uint32_t volatile b = 0x0000BABE;
    __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    load_gdt(&_gdt_gdtr);
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_write(3, 8,  'H', 0, 0xF);
    framebuffer_write(3, 9,  'a', 0, 0xF);
    framebuffer_write(3, 10, 'i', 0, 0xF);
    framebuffer_write(3, 11, '!', 0, 0xF);
    framebuffer_set_cursor(3, 10);
    while (true) b += 1;

    // int col = 0;
    // keyboard_state_activate();
    // while (true){
    //     char c;
    //     get_keyboard_buffer(&c);
    //     if (c) framebuffer_write(0, col++, c, 0xF, 0);
    // }
}