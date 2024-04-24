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
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // Framebuffer operations
    framebuffer_clear();
    create_bg();
    write_string(10, 8, "Hello, User!", 0, 0x2);
    write_string(11, 11, "Welcome to Macrosoft Winning OS!", 0, 0x2);
    framebuffer_set_cursor(0, 0);

    // Initialize filesystem FAT32
    initialize_filesystem_fat32();

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

    // Write folder
    // struct FAT32DriverRequest req = {
    //     .name = "inifold",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size = 0
    // };
    // uint8_t write_dir = write(req);
    // write_dir++;
    // // Modifikasi untuk testing read directory
    // if (write_dir != 0) {
    //     // Menulis direktori gagal
    //         write_string(15, 11, "Direktori gagal!", 0, 0x2);
    // }

    // // Write file
    // char* bufstr = "akldsaldldkadjkldxklajdksadiwqldakldaskldasd";
    // struct FAT32DriverRequest req2 = {
    //     .buf = bufstr,
    //     .name = "inifile2",
    //     .parent_cluster_number = 5,
    //     .buffer_size = 0
    // };
    // uint8_t write_file = write(req2);
    // write_file++;

    //  // Membaca direktori yang baru ditulis (Testing Read Directory)
    // struct FAT32DriverRequest read_req = {
    //     // .buf = &fat32_driverstate.dir_table_buf,  // Buffer untuk menyimpan hasil bacaan
    //     .name = "inifold",  // Nama direktori yang akan dibaca
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size = sizeof(struct FAT32DirectoryTable)  // Ukuran buffer adalah ukuran dari FAT32DirectoryTable
    // };
    // int8_t read_result = read_directory(read_req);
    // if (read_result != 0) {
    //     // Membaca direktori gagal, lakukan penanganan kesalahan di sini jika diperlukan
    // } else {
    //     // Proses hasil pembacaan direktori di sini
    //     // Misalnya, Anda dapat mencetak informasi direktori yang dibaca ke layar
    //     struct FAT32DirectoryTable* dir_table = (struct FAT32DirectoryTable*) read_req.buf;
    //     // TO DO : lakukan sesuatu dengan dir_table
    // }

    int col = 0;
    keyboard_state_activate();
    while (true){
        char c;
        get_keyboard_buffer(&c);
        if (c) framebuffer_write(0, col++, c, 0xF, 0);
    }
}