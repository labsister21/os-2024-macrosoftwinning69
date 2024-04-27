#include <stdint.h>
#include <string.h>
#include "../header/filesystem/fat32.h"

#define BLOCK_COUNT 16

char currentdir[255] = "/";
uint8_t currentdirlen = 1;

uint32_t currentdirnode = 1;

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

void sys_mkdir(const char* path) {
    syscall(9, (uint32_t) path, strlen(path), 0);
}

// Shell status
struct ShellStatus {
    uint8_t is_open;
};

struct ShellStatus shell_status = {
    .is_open = 0
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

// Main shell program
int main(void) {
    char* prompt_string = "Macro@OS-2024 ~ ";
    syscall(6, (uint32_t) prompt_string, strlen(prompt_string), (uint32_t) 0xA);

    char buf;
    while (true) {
        syscall(4, (uint32_t) &buf, 0, 0);

        if (buf != '\0') {
            if (buf == 'p') {
                syscall(7, 0x400000, 0, 0);
            }
            syscall(5, (uint32_t) &buf, 0x7, 0);
        }
    }

    return 0;
}

void ls(struct FAT32DriverRequest *request, char *dirname)
{
    strncpy(request->name, dirname, 8);
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    int8_t retval = sys_read_directory(request);
    if (retval != 0)
    {
        puts(dirname);
        puts(": No such file or directory\n");
        return;
    }
    uint32_t offset = get_directory_first_child_offset(request->buf);
    struct FAT32DirectoryEntry *entry = get_directory_entry(request->buf, offset);
    while (entry->name[0] != 0){
        if (entry->attribute == 0x10){ //directory
            request->parent_cluster_number = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
            sys_read_next_directory(request);
            offset = 0;
            entry = get_directory_entry(request->buf, offset);
            continue;
        }
        if (entry->attribute == 0x20){ //regular file
            puts(entry->name);
        }
        else{
            puts_color(entry->name, 0x9);
        }
        puts("  ");
        offset += sizeof(struct FAT32DirectoryEntry);
        entry = get_directory_entry(request->buf, offset);
    }
    puts("\n");
}

// Command untuk membuat direktori bari pada terminal
void mkdir(char *dirname) {
    if (strlen(dirname) == 0) {
        puts("mkdir: missing operand\n");
        return;
    }
    sys_mkdir(dirname);
}

// int main(void) {
//     struct ClusterBuffer      cl      = {0};
//     struct FAT32DriverRequest request = {
//         .buf                   = &cl,
//         .name                  = "kano",
//         .ext                   = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = CLUSTER_SIZE * 3,
//     };
//     int32_t retcode;
//     syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
//     if (retcode == 0)
//         syscall(6, (uint32_t) "owo\n", 4, 0xF);

//     char buf;
//     while (true) {
//         syscall(4, (uint32_t) &buf, 0, 0);

//         if (buf != '\0') {
//             syscall(5, (uint32_t) &buf, 0xF, 0);
//         }
//     }

//     return 0;
// }