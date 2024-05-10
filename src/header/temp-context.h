#include <stdint.h>

struct Context {
    struct {
        uint32_t edi;                   // 0
        uint32_t esi;                   // 4
    } __attribute__((packed)) index;
    struct {
        uint32_t ebp;                   // 8
        uint32_t esp;                   // 12
    } __attribute__((packed)) stack;
    struct {
        uint32_t ebx;                   // 16
        uint32_t edx;                   // 20
        uint32_t ecx;                   // 24
        uint32_t eax;                   // 28
    } __attribute__((packed)) general;
    struct {
        uint32_t gs;                    // 32
        uint32_t fs;                    // 36
        uint32_t es;                    // 40
        uint32_t ds;                    // 44
    } __attribute__((packed)) segment;
    uint32_t                eip;        // 48
    uint32_t                eflags;     // 52
    struct PageDirectory*   page_directory_virtual_addr;
};