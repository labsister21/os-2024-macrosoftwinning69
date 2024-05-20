#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/interrupt/idt.h"
#include "header/interrupt/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/disk.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/memory/paging.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"
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
    // Test paging
    // paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0x600000);
    // *((uint8_t*) 0x900000) = 1;

    // Load GDT
    load_gdt(&_gdt_gdtr);

    // Initialize IDT and keyboard
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // Initialize filesystem FAT32
    initialize_filesystem_fat32();

    // Setup user mode
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    // Write bin folder
    // struct FAT32DriverRequest bin = {
    //     .name = "bin",
    //     .ext = "\0\0\0",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size = 0
    // };
    // write(bin);
    
    // Write shell into memory
    uint32_t BIN_CLUSTER_NUMBER = 5;
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = BIN_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();

    // Create first process (shell)
    process_create_user_process(request);

    // Start scheduler
    scheduler_init();
    scheduler_switch_to_next_process();
}