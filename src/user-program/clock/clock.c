#include <stdint.h>
#include "../../header/filesystem/fat32.h"
#include "../SYSCALL_LIBRARY.h"
#include "../utils.h"

// System call function
void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

char* itoa[] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
};

int main(void) {
    // Activate keyboard input
    syscall(SYSCALL_ACTIVATE_KEYBOARD, 0, 0, 0);

    // Main program loop
    while (true) {
        // Construct clock struct
        struct SyscallClockTimeArgs clock_time;

        // Get clock time
        syscall(SYSCALL_GET_CLOCK_TIME, (uint32_t)&clock_time, 0, 0);

        // Print clock time
        struct StringN time_str;
        stringn_create(&time_str);

        // Hour
        uint8_t hour = clock_time.hour;
        stringn_appendstr(&time_str, itoa[hour]);

        // Separator
        stringn_appendstr(&time_str, ":");

        // Minute
        uint8_t minute = clock_time.minute;
        stringn_appendstr(&time_str, itoa[minute]);

        // Separator
        stringn_appendstr(&time_str, ":");

        // Second
        uint8_t second = clock_time.second;
        stringn_appendstr(&time_str, itoa[second]);

        // Print to screen
        struct SyscallPutsAtArgs time = {
            .buf = time_str.buf,
            .count = time_str.len,
            .fg_color = BIOS_BLACK,
            .bg_color = BIOS_LIGHT_CYAN,
            .row = 24,
            .col = 71,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&time, 0, 0);
    }
}