#include "header/cpu/interrupt.h"
#include "header/cpu/portio.h"

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}