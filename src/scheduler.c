#include "header/scheduler/scheduler.h"

void activate_timer_interrupt(void) {
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) (pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) ((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
    __asm__ volatile("sti");
}

void scheduler_init(void) {
    // Initialize scheduler
    // Set current running process to NULL
    // current_running_pcb = NULL;
}

// void scheduler_save_context_to_current_running_pcb(struct Context ctx) {

// }

// void scheduler_switch_to_next_process(void) {

// }