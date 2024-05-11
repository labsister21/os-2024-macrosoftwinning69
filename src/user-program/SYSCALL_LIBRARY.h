#ifndef __SYSCALL_LIBRARY_H
#define __SYSCALL_LIBRARY_H

// Syscall numbers
#define SYSCALL_READ                    0
#define SYSCALL_READ_DIRECTORY          1
#define SYSCALL_WRITE                   2
#define SYSCALL_DELETE                  3
#define SYSCALL_GETCHAR                 4
#define SYSCALL_PUTCHAR                 5
#define SYSCALL_PUTS                    6
#define SYSCALL_PUTS_AT                 7
#define SYSCALL_ACTIVATE_KEYBOARD       8
#define SYSCALL_DEACTIVATE_KEYBOARD     9
#define SYSCALL_KEYBOARD_PRESS_SHIFT    10
#define SYSCALL_KEYBOARD_PRESS_CTRL     11
#define SYSCALL_CLEAR_SCREEN            12
#define SYSCALL_SET_CURSOR              13
#define SYSCALL_GET_CURSOR_ROW          14
#define SYSCALL_GET_CURSOR_COL          15
#define SYSCALL_READ_CLUSTER            16
#define SYSCALL_TERMINATE_PROCESS       17
#define SYSCALL_GET_MAX_PROCESS_COUNT   18
#define SYSCALL_GET_PROCESS_INFO        19

// BIOS colors
#define BIOS_BLACK                     0x0
#define BIOS_BLUE                      0x1
#define BIOS_GREEN                     0x2
#define BIOS_CYAN                      0x3
#define BIOS_RED                       0x4
#define BIOS_MAGENTA                   0x5
#define BIOS_BROWN                     0x6
#define BIOS_LIGHT_GRAY                0x7
#define BIOS_DARK_GRAY                 0x8
#define BIOS_LIGHT_BLUE                0x9
#define BIOS_LIGHT_GREEN               0xA
#define BIOS_LIGHT_CYAN                0xB
#define BIOS_LIGHT_RED                 0xC
#define BIOS_LIGHT_MAGENTA             0xD
#define BIOS_YELLOW                    0xE
#define BIOS_WHITE                     0xF

// Itoa table
char* itoa[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49"
};

// Extended scancode handling
#define EXT_BUFFER_NONE               0
#define EXT_BUFFER_UP                 1
#define EXT_BUFFER_DOWN               2
#define EXT_BUFFER_LEFT               3
#define EXT_BUFFER_RIGHT              4

// Helper structs
struct SyscallPutsArgs {
    char* buf;
    uint32_t count;
    uint32_t fg_color;
    uint32_t bg_color;
};

struct SyscallPutsAtArgs {
    char* buf;
    uint32_t count;
    uint32_t fg_color;
    uint32_t bg_color;
    uint8_t row;
    uint8_t col;
};

struct SyscallProcessInfoArgs {
    // Metadata
    uint32_t pid;
    char name[32];
    char state[8];

    // Flag to check if pid-th slot has a process
    bool process_exists;

    // Memory
    uint32_t page_frame_used_count;
};

#endif