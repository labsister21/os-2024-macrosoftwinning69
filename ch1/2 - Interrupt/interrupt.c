#include "interrupt.h"

void activate_keyboard_interupt(void){
    out(PIC1_DATA, IN(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}