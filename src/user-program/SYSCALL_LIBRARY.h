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
#define SYSCALL_GET_KEYBOARD_BORDERS    10
#define SYSCALL_SET_KEYBOARD_BORDERS    11
#define SYSCALL_KEYBOARD_PRESS_SHIFT    12
#define SYSCALL_KEYBOARD_PRESS_CTRL     13
#define SYSCALL_CLEAR_SCREEN            14
#define SYSCALL_SET_CURSOR              15
#define SYSCALL_GET_CURSOR_ROW          16
#define SYSCALL_GET_CURSOR_COL          17
#define SYSCALL_READ_CLUSTER            18
#define SYSCALL_TERMINATE_PROCESS       19
#define SYSCALL_CREATE_PROCESS          20
#define SYSCALL_GET_MAX_PROCESS_COUNT   21
#define SYSCALL_GET_PROCESS_INFO        22
#define SYSCALL_GET_CLOCK_TIME          23
#define SYSCALL_GET_IS_SHELL_OPEN       24
#define SYSCALL_SET_IS_SHELL_OPEN       25
#define SYSCALL_FIND_FILE               30

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

struct SyscallKeyboardBordersArgs {
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
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

struct SyscallClockTimeArgs {
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    unsigned char day;
    unsigned char month;
    unsigned char year;
};

#endif