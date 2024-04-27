#include "header/interrupt/interrupt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/cpu/gdt.h"
#include "header/filesystem/fat32.h"
#include "header/text/framebuffer.h"
#include "header/kernel-entrypoint.h"

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8) out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

void main_interrupt_handler(struct InterruptFrame frame) {
    switch (frame.int_number) {
        case (PIC1_OFFSET + IRQ_KEYBOARD):
            keyboard_isr();
            break;
        case (0x30):
            syscall(frame);
            break;
    }
}

void puts(char* buf, uint32_t count, uint32_t color) {
    while (count && *buf != '\0') {
        framebuffer_write(0, keyboard_state.col++, *buf, color, 0);
        framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
        buf++;
        count--;
    }
}

void syscall(struct InterruptFrame frame) {
    if (frame.cpu.general.eax == 0) {
        *((int8_t*) frame.cpu.general.ecx) = read(
            *(struct FAT32DriverRequest*) frame.cpu.general.ebx
         );
    } else if (frame.cpu.general.eax == 4) {
        keyboard_state_activate();
        
        // TODO: getchar()
        get_keyboard_buffer((char*) frame.cpu.general.ebx);

    } else if (frame.cpu.general.eax == 5)  {
        puts(
            (char*) frame.cpu.general.ebx,
            1,
            frame.cpu.general.ecx
        );
    } else if (frame.cpu.general.eax == 6) {
        puts(
            (char*) frame.cpu.general.ebx, 
            frame.cpu.general.ecx, 
            frame.cpu.general.edx
        ); // Assuming puts() exist in kernel
    } else if (frame.cpu.general.eax == 7) {
        set_tss_kernel_current_stack();
        kernel_execute_user_program((uint32_t*) frame.cpu.general.ebx);
    }
}
