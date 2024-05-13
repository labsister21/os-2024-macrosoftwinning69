#include <stdint.h>
#include "../header/stdlib/string.h"

// Helper structs
struct StringN {
    char buf[256];
    uint32_t len;
};

struct StringNList_Node {
    struct StringN data;
    struct StringNList_Node* next;
};

struct StringNList {
    struct StringNList_Node* head;
    struct StringNList_Node* tail;
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

bool strcmp(char* str1, char* str2) {
    // Get lengths of str1 and str2
    uint32_t len1 = strlen(str1);
    uint32_t len2 = strlen(str2);

    // If lengths are not equal, return false
    if (len1 != len2) return false;
    
    // Compare each character
    while (*str1) {
        if (*str1 != *str2) return false;
        str1++;
        str2++;
    }
    
    return true;
}

void strcpy(char* dest, char* src) {
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

// StringN operations
void stringn_create(struct StringN* str) {
    memset(str->buf, '\0', 256);
    str->len = 0;
}

void stringn_appendchar(struct StringN* str, char c) {
    str->buf[str->len] = c;
    str->len++;
}

void stringn_appendstr(struct StringN* str, char* buf) {
    while (*buf) {
        str->buf[str->len] = *buf;
        str->len++;
        buf++;
    }
}