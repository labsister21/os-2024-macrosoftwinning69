#include <stdint.h>

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