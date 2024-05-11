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
    // __asm__ volatile("sti");
}

void scheduler_init(void) {
    activate_timer_interrupt();
}

void scheduler_save_context_to_current_running_pcb(struct Context ctx) {
    // Get current running process
    struct ProcessControlBlock* pcb = process_get_current_running_pcb_pointer();

    // Return if no process is running
    if (pcb != NULL) {
        // Change state from RUNNING to READY
        pcb->metadata.state = PROCESS_STATE_READY;
        
        // Get context of current process
        struct Context* process_context = &(pcb->context);

        // Save context to current running process
        process_context->cpu = ctx.cpu;
        process_context->eip = ctx.eip;
        process_context->eflags = ctx.eflags;
    }
}

void scheduler_switch_to_next_process(void) {
    // Get index of current process
    uint32_t index = process_manager_state.current_running_pid;

    // If no process is running, start from 0
    if (index == NO_PROCESS_RUNNING) index = 0;

    // Find next process to run
    struct ProcessControlBlock next_process;
    while (true) {
        index = (index + 1) % PROCESS_COUNT_MAX;
        if (process_manager_state.process_list_used[index] == true) {
            next_process = _process_list[index];
            _process_list[index].metadata.state = PROCESS_STATE_RUNNING;
            process_manager_state.current_running_pid = index;
            break;
        }
    }

    // Switch to next process
    paging_use_page_directory(next_process.context.page_directory_virtual_addr);
    process_context_switch(next_process.context);
}