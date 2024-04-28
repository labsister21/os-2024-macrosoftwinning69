#include <stdint.h>
#include "../header/stdlib/string.h"

// Helper structs
struct StringN {
    char buf[256];
    uint32_t len;
};

// Helper functions
uint32_t strlen(char* buf) {
    uint32_t count = 0;
    while (*buf != '\0') {
        count++;
        buf++;
    }
    return count;
}

bool strcmp(const char* str1, const char* str2, int n) {
    int i = 0;
    while (*str1 && (*str1 == *str2) && i < n) {
        str1++;
        str2++;
        i++;
    }
    
    return i==n;
}

void stringn_create(struct StringN* str) {
    memset(str->buf, '\0', 256);
    str->len = 0;
}

void stringn_appendchar(struct StringN* str, char c) {
    str->buf[str->len] = c;
    str->len++;
}