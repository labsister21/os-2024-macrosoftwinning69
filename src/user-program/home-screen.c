#include <stdint.h>
#include "../header/filesystem/fat32.h"

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

uint32_t strlen(char* buf) {
    uint32_t count = 0;
    while (*buf != '\0') {
        count++;
        buf++;
    }
    return count;
}

int main(void) {
    char* welcome = "what's this OwO";
    syscall(6, (uint32_t) welcome, strlen(welcome), 0xF);

    char buf;
    while (true) {
        syscall(4, (uint32_t) &buf, 0, 0);

        if (buf != '\0') {
            if (buf == 'p') {
                syscall(7, 0, 0, 0);
            }
            syscall(5, (uint32_t) &buf, 0xF, 0);
        }
    }

    return 0;
}