#include <stdint.h>
#include "../header/filesystem/fat32.h"
#include "SYSCALL_LIBRARY.h"
#include "shell-background.h"

// #define BLOCK_COUNT 16

// static uint32_t currenDir = ROOT_CLUSTER_NUMBER;
// static struct FAT32DirectoryTable curTable;
// static char curDirName[300] = "/\0";
// static struct FAT32DirectoryTable rootTable;

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

// void sys_mkdir(const char* path) {
//     syscall(9, (uint32_t) path, strlen(path), 0);
// }

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

// bool strcmp(const char* str1, const char* str2, int n) {
//     int i = 0;
//     while (*str1 && (*str1 == *str2) && i < n) {
//         str1++;
//         str2++;
//         i++;
//     }
    
//     return i==n;
// }

// Procedures
void shell_create_bg() {
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 80; j++) {
            struct SyscallPutsAtArgs args = {
                .buf = " ",
                .count = 1,
                .fg_color = 0x0,
                .bg_color = bg[i][j],
                .row = i,
                .col = j
            };

            syscall(SYSCALL_PUTS_AT, (uint32_t) &args, 0, 0);
        }
    }
}

// Main shell program
int main(void) {
    // Activate keyboard input
    syscall(SYSCALL_ACTIVATE_KEYBOARD, 0, 0, 0);

    // Create shell background
    shell_create_bg();
    
    // Write shell prompt
    struct SyscallPutsArgs args = {
        .buf = "Macro@OS-2024 ~ ",
        .count = strlen(args.buf),
        .fg_color = 0xA,
        .bg_color = 0x0
    };

    syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

    // Behavior variables
    char buf;
    // bool press_shift;
    // bool press_ctrl;
    struct SyscallPutsArgs puts_args = {
        .buf = &buf,
        .count = 1,
        .fg_color = 0x7,
        .bg_color = 0
    };
    while (true) {
        // Get if user is pressing ctrl
        // syscall(SYSCALL_KEYBOARD_PRESS_CTRL, (uint32_t) &press_ctrl, 0, 0);

        // Get input character from keyboard
        syscall(SYSCALL_GETCHAR, (uint32_t) &buf, 0, 0);
        
        if (buf != '\0') {
            syscall(SYSCALL_PUTCHAR, (uint32_t) &puts_args, 0, 0);
        }

        // // Conditional logic depending on whether shell is open or not
        // if (!shell_status.is_open) {
        //     // Handler if user press ctrl + s
        //     if (press_ctrl && buf == 's') {
        //         // Set shell to open
        //         shell_status.is_open = true;
                
        //         // Clear screen and print prompt
        //         syscall(SYSCALL_CLEAR_SCREEN, 0, 0, 0);
                
        //         char* prompt_string = "Macro@OS-2024 ~ ";

        //         syscall(SYSCALL_PUTS, (uint32_t) prompt_string, strlen(prompt_string), (uint32_t) 0xA);
        //     }
        // } else {
        //     // Handler if user press ctrl + s
        //     if (press_ctrl && buf == 's') {
        //         // Set shell to open
        //         shell_status.is_open = false;

        // framebuffer_clear();
        // create_bg();
        // write_string(10, 8, "Hello, User!", 0, 0x2);
        // write_string(11, 11, "Welcome to Macrosoft Winning OS!", 0, 0x2);
        //         framebuffer_set_cursor(0, 0);
        //     } else if (buf != '\0') {
        //         syscall(SYSCALL_PUTCHAR, (uint32_t) &buf, 0x7, 0);
        //     }
        // }

    }

    return 0;
}

// void ls(){
//     uint32_t retcode;
//     struct FAT32DriverRequest request2 = {
//             .buf                   = &curTable,
//             .name                  = "\0\0\0\0\0\0\0\0",
//             .ext                   = "\0\0\0",
//             .parent_cluster_number = currenDir,
//             .buffer_size           = 0,
//     };
//     for(int i = 0; i < slen(curTable.table[0].name); i++){
//         request2.name[i] = curTable.table[0].name[i];
//     }
//     syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
//     struct FAT32DirectoryTable table = curTable;
//     for(int i = 1 ; i < 64 ; i++){
//         if(table.table[i].attribute || table.table[i].cluster_high || table.table[i].cluster_low){
//             int temp_int = 0;
//             if(slen(table.table[i].name) > 8){
//                 temp_int = 8;
//             } else {
//                 temp_int = slen(table.table[i].name);
//             }
//             syscall(5, (uint32_t) table.table[i].name, temp_int, 0xf);
//             temp_int = 0;
//             if(slen(table.table[i].ext) > 3){
//                 temp_int = 3;
//             } else {
//                 temp_int = slen(table.table[i].ext);
//             }
//             if(temp_int > 0){
//                 syscall(5, (uint32_t) ".", 1, 0xf);
//             }
//             syscall(5, (uint32_t) table.table[i].ext, temp_int, 0xf);
//             syscall(5, (uint32_t) "\n", 1, 0xf);
//         }
//     }
// }

// // Command untuk membuat direktori bari pada terminal
// void mkdir(char *dirname) {
//     if (strlen(dirname) == 0) {
//         puts("mkdir: missing operand\n");
//         return;
//     }
//     sys_mkdir(dirname);
// }

// void cd(char* filename){
//     // check if ..
//     if (strcmp("..\0", filename, 2)){
//         if (currenDir == ROOT_CLUSTER_NUMBER){
//             syscall(5, (uint32_t) "Already at root directory\n", 27, 0xf);
//             return;
//         }
//         // currenDir = (uint32_t)curTable.table[0].cluster_high << 16 | curTable.table[0].cluster_low; // target cluster number
    
//         // start from root so curtable is set to root table
//         struct FAT32DirectoryTable table = rootTable;

//         // iterate from root to parent
//         char prev[300] = "\0";
//         for(int i = 0 ; i < 300 ; i++){
//             prev[i] = '\0';
//         }
//         prev[0] = '/';
//         int i = 0;
//         char pre[300] = "\0";
//         char post[300] = "\0";
//         // initialize post as curDirName
//         for (int i = 0 ; i < slen(curDirName); i++){
//             post[i] = curDirName[i];
//         }
//         while(!clusterExist(table, currenDir)){
//             // save current directory name
//             char tempChar[300] = "\0";
//             for (int j = 0 ; j < 300 ; j++){
//                 tempChar[j] = '\0';
//             }
//             for (int j = 0 ; j < slen(post); j++){
//                 tempChar[j] = post[j];
//             }
//             splitPath(tempChar, pre, post);
//             i = 0;
//             while(pre[i]!='\0'){
//                 prev[slen(prev)] = pre[i];
//                 i++;
//             }
//             prev[slen(prev)] = '/';

//             // split pre into name and ext
//             char name[9] = "\0\0\0\0\0\0\0\0\0";
//             char ext[4] = "\0\0\0\0";
//             readfname(pre, name, ext);
//             uint8_t retcode = 0;

//             // int temp_int = 1;
//             // while(!(strcmp(curTable.table[temp_int].name, name, slen(name)) && strcmp(curTable.table[temp_int].ext, ext, slen(ext)))){
//             //     temp_int++;
//             // }
//             struct FAT32DriverRequest tempRequest = {
//                 .buf                    = &table,
//                 .name                   = "\0\0\0\0\0\0\0\0",
//                 .ext                    = "\0\0\0",
//                 .parent_cluster_number  = (table.table[0].cluster_high << 16 | table.table[0].cluster_low),
//                 .buffer_size            = 0,
//             };

//             // clear tempRequest name and ext
//             for (int j = 0; j < 8; j++){
//                 tempRequest.name[j] = '\0';
//             }
//             for (int j = 0; j < 3; j++){
//                 tempRequest.ext[j] = '\0';
//             }

//             // set it to name and ext
//             int temp_int = 0;
//             if (slen(name) > 8){
//                 temp_int = 8;
//             } else {
//                 temp_int = slen(name);
//             }
//             for (int j = 0; j < temp_int; j++){
//                 tempRequest.name[j] = name[j];
//             }
//             if (slen(ext) > 3){
//                 temp_int = 3;
//             } else {
//                 temp_int = slen(ext);
//             }
//             for (int j = 0; j < temp_int; j++){
//                 tempRequest.ext[j] = ext[j];
//             }
//             syscall(1, (uint32_t) &tempRequest, (uint32_t)&retcode, 0);
//             if (retcode != 0){
//                 syscall(5, (uint32_t) "Directory not found\n", 21, 0xf);
//                 return;
//             }
//         }
//         // save current directory name
//         for(int j = 0 ; j < 300 ; j++){
//             curDirName[j] = '\0';
//         }
//         curDirName[0] = '/';
//         for(int j = 1 ; j < slen(prev) && slen(prev) > 1 ; j++){
//             curDirName[j] = prev[j];
//         }
//         curTable = table;
//         currenDir = (table.table[0].cluster_high << 16 | table.table[0].cluster_low);

//         return;
//     } else if (filename[0] == '/'){
//         // start from root so curtable is set to root table
//         struct FAT32DirectoryTable table = curTable;

//         // iterate from root to parent
//         char temp[300] = "\0";
//         temp[0] = '/';
//         char pre[300] = "\0";
//         char post[300] = "\0";
//         // initialize post as curDirName
//         for (int i = 1 ; i < slen(curDirName); i++){
//             temp[i] = curDirName[i];
//         }
//         temp[slen(curDirName)] = '\0';
//         for (int i = 0 ; i < slen(filename); i++){
//             post[i] = filename[i];
//         }
//         while(slen(post)-1!=0){
//             // save current directory name
//             char tempChar[300] = "\0";
//             for (int j = 0 ; j < 300 ; j++){
//                 tempChar[j] = '\0';
//             }
//             for (int j = 0 ; j < slen(post); j++){
//                 tempChar[j] = post[j];
//             }
//             splitPath(tempChar, pre, post);
//             /* i = 0;
//             int tint = slen(temp);
//             while(pre[i]!='\0'){
//                 temp[tint] = pre[i];
//                 i++;
//                 tint++;
//             }
//             temp[tint] = '/';
//             temp[tint+1] = '\0'; */

//             // split pre into name and ext
//             char name[9] = "\0\0\0\0\0\0\0\0\0";
//             char ext[4] = "\0\0\0\0";
//             readfname(pre, name, ext);
//             uint8_t retcode = 0;

//             // int temp_int = 1;
//             // while(!(strcmp(curTable.table[temp_int].name, name, slen(name)) && strcmp(curTable.table[temp_int].ext, ext, slen(ext)))){
//             //     temp_int++;
//             // }
//             struct FAT32DriverRequest tempRequest = {
//                 .buf                    = &table,
//                 .name                   = "\0\0\0\0\0\0\0\0",
//                 .ext                    = "\0\0\0",
//                 .parent_cluster_number  = (table.table[0].cluster_high << 16 | table.table[0].cluster_low),
//                 .buffer_size            = 0,
//             };

//             // clear tempRequest name and ext
//             for (int j = 0; j < 8; j++){
//                 tempRequest.name[j] = '\0';
//             }
//             for (int j = 0; j < 3; j++){
//                 tempRequest.ext[j] = '\0';
//             }

//             // set it to name and ext
//             int temp_int = 0;
//             if (slen(name) > 8){
//                 temp_int = 8;
//             } else {
//                 temp_int = slen(name);
//             }
//             for (int j = 0; j < temp_int; j++){
//                 tempRequest.name[j] = name[j];
//             }
//             if (slen(ext) > 3){
//                 temp_int = 3;
//             } else {
//                 temp_int = slen(ext);
//             }
//             for (int j = 0; j < temp_int; j++){
//                 tempRequest.ext[j] = ext[j];
//             }
//             syscall(1, (uint32_t) &tempRequest, (uint32_t)&retcode, 0);
//             if (retcode != 0){
//                 syscall(5, (uint32_t) "Directory not found\n", 21, 0xf);
//                 return;
//             }
//             for (int i = 0; i < slen(pre); i++){
//                 temp[slen(temp)] = pre[i];
//             }
//             temp[slen(temp)] = '/';
//         }
//         // save current directory name
//         for(int j = 0 ; j < 300 ; j++){
//             curDirName[j] = '\0';
//         }
//         for(int j = 0 ; j < slen(temp) ; j++){
//             curDirName[j] = temp[j];
//         }
//         curTable = table;
//         currenDir = (table.table[0].cluster_high << 16 | table.table[0].cluster_low);

//         return;
//     }

//     char name[9] = "\0\0\0\0\0\0\0\0\0";
//     char ext[4] = "\0\0\0\0";
//     struct FAT32DirectoryTable table = curTable;
//     readfname(filename, (char *)name, (char *)ext);
//     struct FAT32DriverRequest request = {
//             .buf = &table,
//             .name = "\0\0\0\0\0\0\0\0",
//             .ext = "\0\0\0",
//             .parent_cluster_number = currenDir,
//             .buffer_size = 0
//     };
//     int32_t retcode = 0;
//     for(int i = 0; i < slen(name); i++){
//         request.name[i] = name[i];
//     }
//     for(int i = 0; i < slen(ext); i++){
//         request.ext[i] = ext[i];
//     }
//     syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
//     if(retcode != 0){
//         syscall(5, (uint32_t) "No such directory\n", 18, 0xf);
//         return;
//     }
//     if(!strcmp(curDirName + 1,name,slen(name))){
//         for (int i = 0; i < slen(name); i++){
//             curDirName[slen(curDirName)] = name[i];
//         }
//         curDirName[slen(curDirName)] = '/';
//     }
    
//     curTable = table;
//     currenDir = (table.table[0].cluster_high << 16 | table.table[0].cluster_low);
// }

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