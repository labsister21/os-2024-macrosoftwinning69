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

// Analog clock arrow coordinates
struct Point {
    uint8_t x;
    uint8_t y;
};

struct Point CLOCK_12[] = {{63, 6}, {63, 5}, {63, 4}, {0, 0}};
struct Point CLOCK_1[] = {{64, 6}, {65, 6}, {66, 5}, {67, 5}, {68, 4}, {69, 4}, {0, 0}};
struct Point CLOCK_2[] = {{64, 7}, {65, 7}, {66, 6}, {67, 6}, {68, 5}, {69, 5}, {0, 0}};
struct Point CLOCK_2_5[] = {{64, 7}, {65, 7}, {66, 6}, {67, 6}, {68, 6}, {69, 6}, {0, 0}};
struct Point CLOCK_3[] = {{64, 7}, {65, 7}, {66, 7}, {67, 7}, {68, 7}, {69, 7}, {0, 0}};
struct Point CLOCK_4[] = {{64, 7}, {65, 7}, {66, 8}, {67, 8}, {68, 8}, {69, 8}, {0, 0}};
struct Point CLOCK_5[] = {{64, 7}, {65, 7}, {66, 8}, {67, 8}, {68, 9}, {69, 9}, {0, 0}};
struct Point CLOCK_5_5[] = {{64,8}, {65, 8}, {66, 9}, {67, 9}, {68, 10}, {69, 10}, {0, 0}};
struct Point CLOCK_6[] = {{63, 8}, {63, 9}, {63, 10}, {0, 0}};
struct Point CLOCK_7[] = {{62, 8}, {61, 8}, {60, 9}, {59, 9}, {58, 10}, {59, 10}, {0, 0}};
struct Point CLOCK_8[] = {{62, 7}, {61, 7}, {60, 8}, {59, 8}, {58, 9}, {59, 9}, {0, 0}};
struct Point CLOCK_8_5[] = {{62, 7}, {61, 7}, {60, 8}, {59, 8}, {58, 8}, {57, 8}, {0, 0}};
struct Point CLOCK_9[] = {{62, 7}, {61, 7}, {60, 7}, {59, 7}, {58, 7}, {57, 7}, {0, 0}};
struct Point CLOCK_10[] = {{62, 7}, {61, 7}, {60, 6}, {59, 6}, {58, 5}, {57, 5}, {0, 0}};
struct Point CLOCK_11[] = {{62, 6}, {61, 6}, {60, 5}, {59, 5}, {58, 4}, {57, 4}, {0, 0}};
struct Point CLOCK_11_5[] = {{62, 6}, {61, 6}, {61, 5}, {60, 5}, {60, 4}, {59, 4}, {0, 0}};

uint8_t p_len(struct Point* p) {
    uint8_t i = 0;
    while (p[i].x != 0 && p[i].y != 0) i++;
    return i;
}

uint8_t last_sec;

// Analog clock colors
#define CLOCK_BORDER    BIOS_BLACK
#define CLOCK_BG        BIOS_LIGHT_GRAY
#define CLOCK_HOUR      BIOS_RED
#define CLOCK_MINUTE    BIOS_BLACK
#define CLOCK_SECOND    BIOS_GREEN

void clear_seconds_bar(struct SyscallClockTimeArgs clock_time) {
    for (uint8_t col = 7; col < 66; col++) {
        struct SyscallPutsAtArgs args = {
            .buf = " ",
            .count = 1,
            .fg_color = BIOS_BLACK,
            .bg_color = ((col - 9) <= clock_time.second) ? BIOS_YELLOW : BIOS_BLACK,
            .row = 24,
            .col = col,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args, 0, 0);
    }
    struct SyscallPutsAtArgs sec = {
        .buf = itoa[clock_time.second],
        .count = 2,
        .fg_color = BIOS_BLACK,
        .bg_color = BIOS_YELLOW,
        .row = 24,
        .col = 66,
    };
    syscall(SYSCALL_PUTS_AT, (uint32_t)&sec, 0, 0);
}

#define CLOCK_CENTER_X 63
#define CLOCK_CENTER_Y 7

bool is_in(uint8_t x, uint8_t y, struct Point* coordinates) {
    for (uint8_t i = 0; i < p_len(coordinates); i++) {
        if (coordinates[i].x == x && coordinates[i].y == y) return true;
    }
    return false;
}

bool is_in_hour(uint8_t x, uint8_t y, struct Point* coordinates) {
    uint8_t limit = (coordinates == CLOCK_12 || coordinates == CLOCK_6) ? 2 : 4;
    for (uint8_t i = 0; i < limit; i++) {
        if (coordinates[i].x == x && coordinates[i].y == y) return true;
    }
    return false;
}

void create_clock(struct SyscallClockTimeArgs clock_time) {
    // Get keyboard borders
    struct SyscallKeyboardBordersArgs borders;
    syscall(SYSCALL_GET_KEYBOARD_BORDERS, (uint32_t)&borders, 0, 0);

    // Get is shell open
    bool shell_open;
    syscall(SYSCALL_GET_IS_SHELL_OPEN, (uint32_t)&shell_open, 0, 0);
    if (shell_open) return;

    // Get second arrow coordinates
    struct Point* secondCoordinates;
    uint8_t sec = (uint8_t) clock_time.second;
    
    if      (sec <= 3)  secondCoordinates = CLOCK_12;
    else if (sec <= 7)  secondCoordinates = CLOCK_1;
    else if (sec <= 11) secondCoordinates = CLOCK_2;
    else if (sec <= 15) secondCoordinates = CLOCK_2_5;
    else if (sec <= 19) secondCoordinates = CLOCK_3;
    else if (sec <= 23) secondCoordinates = CLOCK_4;
    else if (sec <= 27) secondCoordinates = CLOCK_5;
    else if (sec <= 30) secondCoordinates = CLOCK_5_5;
    else if (sec <= 34) secondCoordinates = CLOCK_6;
    else if (sec <= 38) secondCoordinates = CLOCK_7;
    else if (sec <= 42) secondCoordinates = CLOCK_8;
    else if (sec <= 45) secondCoordinates = CLOCK_8_5;
    else if (sec <= 49) secondCoordinates = CLOCK_9;
    else if (sec <= 53) secondCoordinates = CLOCK_10;
    else if (sec <= 57) secondCoordinates = CLOCK_11;
    else                secondCoordinates = CLOCK_11_5;

    // Get minute arrow coordinates
    struct Point* minuteCoordinates;
    uint8_t min = (uint8_t) clock_time.minute;

    if      (min <= 3)  minuteCoordinates = CLOCK_12;
    else if (min <= 7)  minuteCoordinates = CLOCK_1;
    else if (min <= 11) minuteCoordinates = CLOCK_2;
    else if (min <= 15) minuteCoordinates = CLOCK_2_5;
    else if (min <= 19) minuteCoordinates = CLOCK_3;
    else if (min <= 23) minuteCoordinates = CLOCK_4;
    else if (min <= 27) minuteCoordinates = CLOCK_5;
    else if (min <= 30) minuteCoordinates = CLOCK_5_5;
    else if (min <= 34) minuteCoordinates = CLOCK_6;
    else if (min <= 38) minuteCoordinates = CLOCK_7;
    else if (min <= 42) minuteCoordinates = CLOCK_8;
    else if (min <= 45) minuteCoordinates = CLOCK_8_5;
    else if (min <= 49) minuteCoordinates = CLOCK_9;
    else if (min <= 53) minuteCoordinates = CLOCK_10;
    else if (min <= 57) minuteCoordinates = CLOCK_11;
    else                minuteCoordinates = CLOCK_11_5;

    // Get hour arrow coordinates
    struct Point* hourCoordinates;
    uint8_t hour = (uint8_t) ((clock_time.hour + 7) % 12);

    if      (hour == 12 || hour == 0)  hourCoordinates = CLOCK_12;
    else if (hour == 1 || hour == 13)  hourCoordinates = CLOCK_1;
    else if (hour == 2 || hour == 14)  hourCoordinates = CLOCK_2;
    else if (hour == 3 || hour == 15)  hourCoordinates = CLOCK_3;
    else if (hour == 4 || hour == 16)  hourCoordinates = CLOCK_4;
    else if (hour == 5 || hour == 17)  hourCoordinates = CLOCK_5;
    else if (hour == 6 || hour == 18)  hourCoordinates = CLOCK_6;
    else if (hour == 7 || hour == 19)  hourCoordinates = CLOCK_7;
    else if (hour == 8 || hour == 20)  hourCoordinates = CLOCK_8;
    else if (hour == 9 || hour == 21)  hourCoordinates = CLOCK_9;
    else if (hour == 10 || hour == 22) hourCoordinates = CLOCK_10;
    else if (hour == 11 || hour == 23) hourCoordinates = CLOCK_11;

    // Create 5x5 box in top right
    for (uint8_t row = 0; row < 9; row++) {
        for (uint8_t col = 0; col < 17; col++) {
            uint8_t abs_row = 3 + row;
            uint8_t abs_col = 55 + col;

            uint32_t color;
            syscall(SYSCALL_GET_IS_SHELL_OPEN, (uint32_t)&shell_open, 0, 0);
            
            if      (shell_open)                                                color = BIOS_BLACK;
            else if (abs_row == CLOCK_CENTER_Y && abs_col == CLOCK_CENTER_X)    color = BIOS_WHITE;
            else if (is_in(abs_col, abs_row, secondCoordinates))                color = CLOCK_SECOND;
            else if (is_in_hour(abs_col, abs_row, hourCoordinates))             color = CLOCK_HOUR;
            else if (is_in(abs_col, abs_row, minuteCoordinates))                color = CLOCK_MINUTE;
            else                                                                color = CLOCK_BG;

            struct SyscallPutsAtArgs args = {
                .buf = " ",
                .count = 1,
                .fg_color = color,
                .bg_color = color,
                .row = 3 + row,
                .col = 55 + col,
            };
            syscall(SYSCALL_PUTS_AT, (uint32_t)&args, 0, 0);
        }
    }

    // Create row border
    for (uint8_t col = 0; col < 19; col++) {
        if (shell_open) continue;

        struct SyscallPutsAtArgs args_xb = {
            .buf = " ",
            .count = 1,
            .fg_color = CLOCK_BORDER,
            .bg_color = CLOCK_BORDER,
            .row = 2,
            .col = 54 + col,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_xb, 0, 0);
        args_xb.row = 12;
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_xb, 0, 0);
    }

    // Create col border
    for (uint8_t row = 0; row < 11; row++) {
        if (shell_open) continue;

        struct SyscallPutsAtArgs args_yb = {
            .buf = " ",
            .count = 1,
            .fg_color = CLOCK_BORDER,
            .bg_color = CLOCK_BORDER,
            .row = 2 + row,
            .col = 54,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_yb, 0, 0);
        
        args_yb.col = 53;
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_yb, 0, 0);

        args_yb.col = 72;
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_yb, 0, 0);
        
        args_yb.col = 73;
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args_yb, 0, 0);
    }
    /*
    0 1 2 3
    4 5 6 7
    8 9 10 11
    12 13 14 15
    
    16 17 18 19
    20 21 22 23
    24 25 26 27
    28 29 30
    
    31 32 33 34 
    35 36 37 38 
    39 40 41 42 
    43 44 45 
    
    46 47 48 49 
    50 51 52 53 
    54 55 56 57 
    58 59
    */
}

void update_clock(struct SyscallClockTimeArgs clock_time) {
    // Create seconds bar
    if (clock_time.second == 0) {
        clear_seconds_bar(clock_time);
    } else {
        struct SyscallPutsAtArgs args = {
            .buf = " ",
            .count = 1,
            .fg_color = BIOS_BLACK,
            .bg_color = BIOS_YELLOW,
            .row = 24,
            .col = 6 + clock_time.second,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&args, 0, 0);

        struct SyscallPutsAtArgs sec = {
            .buf = itoa[clock_time.second],
            .count = 2,
            .fg_color = BIOS_BLACK,
            .bg_color = BIOS_YELLOW,
            .row = 24,
            .col = 66,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&sec, 0, 0);
    }
}

bool last_shell_open_status = false;
int main(void) {
    // Activate keyboard input
    syscall(SYSCALL_ACTIVATE_KEYBOARD, 0, 0, 0);

    // Get shell status
    syscall(SYSCALL_GET_IS_SHELL_OPEN, (uint32_t)&last_shell_open_status, 0, 0);

    // Main program loop
    bool first_run = true;
    while (true) {
        // Construct clock struct
        struct SyscallClockTimeArgs clock_time;

        // Get clock time
        syscall(SYSCALL_GET_CLOCK_TIME, (uint32_t)&clock_time, 0, 0);

        // Set last second
        if (clock_time.second != last_sec) last_sec = clock_time.second;

        // Print clock time
        struct StringN time_str;
        stringn_create(&time_str);

        // Hour
        uint8_t hour = clock_time.hour;
        stringn_appendstr(&time_str, itoa[(hour + 7) % 24]);

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

        // Print clock date
        struct StringN date_str;
        stringn_create(&date_str);

        // Day
        uint8_t day = clock_time.day;
        stringn_appendstr(&date_str, itoa[day]);

        // Separator
        stringn_appendstr(&date_str, "/");

        // Month
        uint8_t month = clock_time.month;
        stringn_appendstr(&date_str, itoa[month]);

        // Separator
        stringn_appendstr(&date_str, "/");

        // Year
        uint8_t year = clock_time.year;
        stringn_appendstr(&date_str, itoa[year]);

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

        struct SyscallPutsAtArgs date = {
            .buf = date_str.buf,
            .count = date_str.len,
            .fg_color = BIOS_BLACK,
            .bg_color = BIOS_LIGHT_CYAN,
            .row = 23,
            .col = 71,
        };
        syscall(SYSCALL_PUTS_AT, (uint32_t)&date, 0, 0);

        // Create seconds bar
        if (first_run) {
            clear_seconds_bar(clock_time);
            first_run = false;
        } else {
            update_clock(clock_time);
        }

        // Create clock
        create_clock(clock_time);
    }
}