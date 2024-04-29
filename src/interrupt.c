#include "header/interrupt/interrupt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/cpu/gdt.h"
#include "header/filesystem/fat32.h"
#include "header/text/framebuffer.h"
#include "header/kernel-entrypoint.h"
#include "user-program/SYSCALL_LIBRARY.h"

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

void puts(struct SyscallPutsArgs args) {
    // Get arguments
    char* buf = args.buf;
    uint32_t count = args.count;
    uint32_t fg_color = args.fg_color;
    uint32_t bg_color = args.bg_color;
    
    while (count && *buf != '\0') {
        if (*buf == '\n') {     // enter
            if (!(keyboard_state.row == keyboard_state.down_limit)) {
                // maju ke baris berikutnya
                keyboard_state.row++;
                keyboard_state.col = keyboard_state.left_limit;
            }

        } else if (*buf == '\b') {  // backspace
            // hapus karakter sebelumnya jika buffer tidak kosong
            if (keyboard_state.col > keyboard_state.left_limit) {
                keyboard_state.col--;
                framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0x07, 0x00);
            } else if (keyboard_state.row > keyboard_state.up_limit) { // jika posisi kolom adalah 0
                // kembali ke baris sebelumnya dan ke kolom terakhir yang berisi karakter non-spasi
                keyboard_state.row--;
                keyboard_state.col = keyboard_state.last_non_space_col[keyboard_state.row] + 1;
            }
        } else if (*buf == '\t') {  // tab
            // maju ke kolom berikutnya yang merupakan kelipatan 4
            keyboard_state.col = (keyboard_state.col + 4) & ~3;
        } else {
            // menyimpan karakter ascii ke dalam framebuffer
            framebuffer_write(keyboard_state.row, keyboard_state.col, *buf, fg_color, bg_color);
            keyboard_state.last_non_space_col[keyboard_state.row] = keyboard_state.col;
            
            // Update col
            if (!(keyboard_state.row == keyboard_state.down_limit && keyboard_state.col == keyboard_state.right_limit)) {
                if (keyboard_state.col == keyboard_state.right_limit) {
                    keyboard_state.row++;
                    keyboard_state.col = keyboard_state.left_limit;
                } else {
                    keyboard_state.col++;
                }
            }
            // jika karakter bukan spasi, perbarui posisi kolom terakhir yang berisi karakter non-spasi
            // if (*buf != ' ') {
            // }
        }
        buf++;
        count--;
    }

    framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
}

void puts_at(struct SyscallPutsAtArgs args) {
    // Get arguments
    char* buf = args.buf;
    uint32_t count = args.count;
    uint32_t fg_color = args.fg_color;
    uint32_t bg_color = args.bg_color;
    uint8_t row = args.row;
    uint8_t col = args.col;

    // Print to screen
    while (count && *buf != '\0') {
        framebuffer_write(row, col, *buf, fg_color, bg_color);

        buf++;
        col++;
        count--;
    }
}

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        // SYSCALL 0
        case SYSCALL_READ:
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 1
        case SYSCALL_READ_DIRECTORY:
            *((int8_t*) frame.cpu.general.ecx) = read_directory(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 4
        case SYSCALL_GETCHAR:
            // keyboard_state_activate();

            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;

        // SYSCALL 5
        case SYSCALL_PUTCHAR:
            struct SyscallPutsArgs* pointer = (struct SyscallPutsArgs*) frame.cpu.general.ebx;
            pointer->count = 1;

            puts(*pointer);
            break;

        // SYSCALL 6
        case SYSCALL_PUTS:
            struct SyscallPutsArgs* pointer_puts = (struct SyscallPutsArgs*) frame.cpu.general.ebx;

            puts(*pointer_puts);
            break;

        // SYSCALL 7
        case SYSCALL_PUTS_AT:
            struct SyscallPutsAtArgs* pointer_puts_at = (struct SyscallPutsAtArgs*) frame.cpu.general.ebx;

            puts_at(*pointer_puts_at);
            break;

        // SYSCALL 8
        case SYSCALL_ACTIVATE_KEYBOARD:
            keyboard_state_activate();
            break;

        // SYSCALL 9
        case SYSCALL_DEACTIVATE_KEYBOARD:
            keyboard_state_deactivate();
            break;

        // SYSCALL 11
        case SYSCALL_KEYBOARD_PRESS_CTRL:
            *((bool*) frame.cpu.general.ebx) = keyboard_state.press_ctrl;
            break;

        // SYSCALL 12
        case SYSCALL_CLEAR_SCREEN:
            framebuffer_clear();
            break;

        // SYSCALL 13
        case SYSCALL_SET_CURSOR:
            framebuffer_set_cursor(
                frame.cpu.general.ebx,
                frame.cpu.general.ecx
            );
            keyboard_state.row = frame.cpu.general.ebx;
            keyboard_state.col = frame.cpu.general.ecx;
            break;

        // SYSCALL 14
        case SYSCALL_GET_CURSOR_ROW:
            *((uint8_t*) frame.cpu.general.ebx) = keyboard_state.row;
            break;

        // SYSCALL 15
        case SYSCALL_GET_CURSOR_COL:
            *((uint8_t*) frame.cpu.general.ebx) = keyboard_state.col;
            break;

        case SYSCALL_READ_CLUSTER:
            read_clusters(frame.cpu.general.ecx, frame.cpu.general.ebx, 1);
            break;
    }
}
