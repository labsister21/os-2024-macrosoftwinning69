#ifndef __SYSCALL_LIBRARY_H
#define __SYSCALL_LIBRARY_H

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

#endif