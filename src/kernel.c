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
    // paging_free_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0x700000);
    // *((uint8_t*) 0x500000) = 1;

    // Load GDT
    load_gdt(&_gdt_gdtr);

    // Initialize IDT and keyboard
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    // activate_timer_interrupt();

    // Framebuffer operations
    // framebuffer_clear();
    // create_bg();
    // write_string(10, 8, "Hello, User!", 0, 0x2);
    // write_string(11, 11, "Welcome to Macrosoft Winning OS!", 0, 0x2);
    // framebuffer_set_cursor(0, 0);

    // Initialize filesystem FAT32
    initialize_filesystem_fat32();

    // Setup user mode
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    // int8_t xd = read(request);
    // xd++;

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    // kernel_execute_user_program((uint8_t*) 0x0);

    // Create first process (shell)
    process_create_user_process(request);
    // paging_use_page_directory(_process_list[0].context.page_directory_virtual_addr);

    // Start scheduler
    scheduler_init();
    scheduler_switch_to_next_process();
    
    // struct Context ctx = {
    //     .cpu = {
    //         .index = {
    //             .edi = 0x50,
    //             .esi = 0x51,
    //         },
    //         .stack = {
    //             .ebp = _process_list[0].context.cpu.stack.ebp,
    //             .esp = _process_list[0].context.cpu.stack.esp,
    //         },
    //         .general = {
    //             .ebx = 0x54,
    //             .edx = 0x55,
    //             .ecx = 0x56,
    //             .eax = 0x57,
    //         },
    //         .segment = {
    //             .gs = _process_list[0].context.cpu.segment.gs,
    //             .fs = _process_list[0].context.cpu.segment.fs,
    //             .es = _process_list[0].context.cpu.segment.es,
    //             .ds = _process_list[0].context.cpu.segment.ds,
    //         },
        
    //     },
    //     .eip = _process_list[0].context.eip,
    //     .eflags = 0x69,
    //     .page_directory_virtual_addr = _process_list[0].context.page_directory_virtual_addr,
    // };
    // process_context_switch(ctx);

    // uint8_t hi = 0;
    // hi++;
    // kernel_execute_user_program((void*) 0x0);

    // while (true);

    // int col = 0;
    // keyboard_state_activate();
    // while (true){
    //     char c;
    //     get_keyboard_buffer(&c);
    //     if (c) framebuffer_write(0, col++, c, 0xF, 0);
    // }

    // Read directory
    // struct FAT32DirectoryTable buf;
    // memset(&buf, 0, CLUSTER_SIZE);

    // struct FAT32DriverRequest req = {
    //     .buf = &buf,
    //     .name = "nestedf1",
    //     .parent_cluster_number = 4,
    //     .buffer_size = CLUSTER_SIZE
    // };
    // int8_t read_dir = read_directory(req);
    // read_dir++;

    // Read file
    // uint8_t buf[CLUSTER_SIZE * 3];
    // memset(&buf, 0, CLUSTER_SIZE * 3);

    // struct FAT32DriverRequest req2 = {
    //     .buf = &buf,
    //     .name = "daijoubu",
    //     .parent_cluster_number = 4,
    //     .buffer_size = CLUSTER_SIZE * 3
    // };
    // int8_t read_file = read(req2);
    // read_file++;

    // Delete file
    // struct FAT32DriverRequest req6 = {
    //     .name = "lol",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER
    // };
    // int8_t delete6 = delete(req6);
    // delete6++;
}