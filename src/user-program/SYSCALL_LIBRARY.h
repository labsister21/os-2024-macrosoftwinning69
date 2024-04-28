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