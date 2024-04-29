#include <stdint.h>
#include "../../header/filesystem/fat32.h"
#include "../SYSCALL_LIBRARY.h"
#include "../utils.h"
#include "shell-background.h"

// #define BLOCK_COUNT 16

// static uint32_t currenDir = ROOT_CLUSTER_NUMBER;
// static struct FAT32DirectoryTable curTable;
// static char curDirName[300] = "/\0";
// static struct FAT32DirectoryTable rootTable;

void cd(struct StringN folder);

// Filesystem variables
struct FAT32DirectoryTable currentDir;
struct StringN currentDirPath;
uint32_t currentDirCluster;

// Shell properties
#define SHELL_WINDOW_UPPER_HEIGHT 0
#define SHELL_WINDOW_LOWER_HEIGHT 24
#define SHELL_WINDOW_LEFT_WIDTH 0
#define SHELL_WINDOW_RIGHT_WIDTH 79

// Shell commands
#define SHELL_CD "cd"
#define SHELL_LS "ls"
#define SHELL_MKDIR "mkdir"
#define SHELL_CAT "cat"
#define SHELL_CP "cp"
#define SHELL_RM "rm"
#define SHELL_MV "mv"
#define SHELL_FIND "find"

#define SHELL_CLEAR "clear"

#define SHELL_OKEGAS "okegas"

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
    bool is_open;
    uint8_t del_limit;          // Limit for backspace
    uint8_t shell_row;
};

struct ShellStatus shell_status = {
    .is_open = false,
    .shell_row = 0
};

// Procedures
struct StringN create_path_recursive(uint32_t cluster) {
    if (cluster == ROOT_CLUSTER_NUMBER) {
        struct StringN path;
        stringn_create(&path);
        stringn_appendstr(&path, "./");
        return path;
    }
    // Create StringN for current folder
    struct StringN currdir;
    stringn_create(&currdir);

    // Get current folder name
    struct FAT32DirectoryTable dir_table;
    syscall(SYSCALL_READ_CLUSTER, (uint32_t) &dir_table, cluster, 0);

    struct FAT32DirectoryEntry dir_entry = dir_table.table[0];

    // Append folder name to currdir
    stringn_appendstr(&currdir, dir_entry.name);
    stringn_appendchar(&currdir, '/');

    // Get parent folder cluster
    struct FAT32DirectoryEntry parent_entry = dir_table.table[1];
    uint32_t parent_cluster = (parent_entry.cluster_high << 16) | parent_entry.cluster_low;

    // Get parent folder path
    struct StringN parent_path = create_path_recursive(parent_cluster);
    stringn_appendstr(&parent_path, currdir.buf);

    return parent_path;
}

void create_path() {
    struct FAT32DirectoryEntry curr_entry = currentDir.table[0];
    uint32_t cluster = (curr_entry.cluster_high << 16) | curr_entry.cluster_low;

    currentDirPath = create_path_recursive(cluster);
}

void set_current_cluster() {
    struct FAT32DirectoryEntry curr_entry = currentDir.table[0];
    currentDirCluster = (curr_entry.cluster_high << 16) | curr_entry.cluster_low;
}

void shell_create_bg() {
    // Set cursor   
    syscall(SYSCALL_SET_CURSOR, 0, 0, 0);

    // Draw background art
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

    // Write welcome message
    struct SyscallPutsAtArgs welcome1 = {
        .buf = "Hello, User!",
        .count = strlen(welcome1.buf),
        .fg_color = 0x0,
        .bg_color = 0x2,
        .row = 10,
        .col = 8
    };
    syscall(SYSCALL_PUTS_AT, (uint32_t) &welcome1, 0, 0);

    struct SyscallPutsAtArgs welcome2 = {
        .buf = "Welcome to Macrosoft Winning OS!",
        .count = strlen(welcome2.buf),
        .fg_color = 0x0,
        .bg_color = 0x2,
        .row = 11,
        .col = 11
    };
    syscall(SYSCALL_PUTS_AT, (uint32_t) &welcome2, 0, 0);

    struct SyscallPutsAtArgs welcome3 = {
        .buf = "Press Ctrl + S to open the Shell Command Line Interface.",
        .count = strlen(welcome3.buf),
        .fg_color = 0xF,
        .bg_color = 0x4,
        .row = 15,
        .col = 12
    };
    syscall(SYSCALL_PUTS_AT, (uint32_t) &welcome3, 0, 0);

    // Set cursor   
    syscall(SYSCALL_SET_CURSOR, 0, 0, 0);
}

void shell_print_prompt() {
    // Get path and save to currentDirPath
    create_path();
 
    // Prompt handler
    // OS Title
    struct SyscallPutsArgs prompt_args = {
        .buf = "Macrosoft@OS-2024 [",
        .count = strlen(prompt_args.buf),
        .fg_color = 0xA,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &prompt_args, 0, 0);

    // Current directory path
    struct StringN prompt;
    stringn_create(&prompt);
    stringn_appendstr(&prompt, currentDirPath.buf);

    struct SyscallPutsArgs prompt_args2 = {
        .buf = prompt.buf,
        .count = strlen(prompt_args2.buf),
        .fg_color = 0x9,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &prompt_args2, 0, 0);

    // User input prompt
    struct SyscallPutsArgs prompt_args3 = {
        .buf = "] >> ",
        .count = strlen(prompt_args3.buf),
        .fg_color = 0xA,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &prompt_args3, 0, 0);

    // Set delete limit
    syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &shell_status.del_limit, 0, 0);
}

void shell_input_handler(struct StringN input) {
    // Get 0th argument
    struct StringN arg0;
    stringn_create(&arg0);

    uint32_t i;
    for (i = 0; (i < input.len && input.buf[i] != ' '); i++) {
        stringn_appendchar(&arg0, input.buf[i]);
    }

    // Get 1st argument
    struct StringN arg1;
    stringn_create(&arg1);

    uint32_t j;
    for (j = 1; (j < input.len && input.buf[i + j] != ' '); j++) {
        stringn_appendchar(&arg1, input.buf[i + j]);
    }
    
    char* command = arg0.buf;

    if (strcmp(command, SHELL_CD)) {
        cd(arg1);
    } else if (strcmp(command, SHELL_LS)) {

    } else if (strcmp(command, SHELL_MKDIR)) {

    } else if (strcmp(command, SHELL_CAT)) {

    } else if (strcmp(command, SHELL_CP)) {

    } else if (strcmp(command, SHELL_RM)) {

    } else if (strcmp(command, SHELL_MV)) {

    } else if (strcmp(command, SHELL_FIND)) {

    } else if (strcmp(command, SHELL_CLEAR)) {
        syscall(SYSCALL_CLEAR_SCREEN, 0, 0, 0);
        syscall(SYSCALL_SET_CURSOR, 0, 0, 0);
        shell_print_prompt();
    } else if (strcmp(command, SHELL_OKEGAS)) {
        struct SyscallPutsArgs args = {
            .buf = "TABRAK-TABRAK MASUK\nRAPPER KAMPUNG TABRAK MASUK\nMESKI JAUH JARAK PANDANG\nCOBA SEDIKIT MENGAMUK\nKU CIPTAKAN LIRIK DAN BEAT\nSECEPAT KILAT TAPI TAK SEMPIT\nBERDIRI TEGAR WALAUPUN SULIT\nTRA MAMPU BERSAING SILAHKAN PAMIT\nOK GAS-OK GAS\nTAMBAH DUA TORANG GAS",
            .count = strlen(args.buf),
            .fg_color = 0xE,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
    } else {
        struct SyscallPutsArgs args = {
            .buf = "Unknown command! Please enter another command.",
            .count = strlen(args.buf),
            .fg_color = 0xC,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
    }
}

// Main shell program
int main(void) {
    // Activate keyboard input
    syscall(SYSCALL_ACTIVATE_KEYBOARD, 0, 0, 0);

    // Create shell background
    shell_create_bg();

    // Load root directory
    struct FAT32DriverRequest request = {
        .buf = &currentDir,
        .name = "root",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = sizeof(struct FAT32DirectoryTable)
    };
    syscall(SYSCALL_READ_DIRECTORY, (uint32_t) &request, (uint32_t) 0, 0);
    set_current_cluster();

    // Behavior variables
    char buf;
    // bool press_shift;
    bool press_ctrl;
    struct SyscallPutsArgs putchar_args = {
        .buf = &buf,
        .count = 1,
        .fg_color = 0x7,
        .bg_color = 0
    };

    // String for storing shell input
    struct StringN shell_input;
    stringn_create(&shell_input);

    // Main program loop
    while (true) {
        // Get if user is pressing ctrl
        syscall(SYSCALL_KEYBOARD_PRESS_CTRL, (uint32_t) &press_ctrl, 0, 0);

        // Get currently pressed key
        syscall(SYSCALL_GETCHAR, (uint32_t) &buf, 0, 0);

        // Conditional logic depending on whether shell is open or not
        if (!shell_status.is_open) {
            // Handler if user presses ctrl + s
            if (press_ctrl && buf == 's') {
                // Set shell to open
                shell_status.is_open = true;

                // Clear screen and print prompt
                syscall(SYSCALL_CLEAR_SCREEN, 0, 0, 0);
                syscall(SYSCALL_SET_CURSOR, 0, 0, 0);
                shell_print_prompt();
            }
        } else {
            // Handler if user presses ctrl + s
            if (press_ctrl && buf == 's') {
                // Set shell to open
                shell_status.is_open = false;

                // Create background
                shell_create_bg();
            } else if (buf == '\n')  {
                // Print newline to screen
                syscall(SYSCALL_PUTCHAR, (uint32_t) &putchar_args, 0, 0);

                // Handle shell input
                shell_input_handler(shell_input);

                // Print extra newline
                if (!strcmp(shell_input.buf, SHELL_CLEAR)) {
                    syscall(SYSCALL_PUTCHAR, (uint32_t) &putchar_args, 0, 0);
                    syscall(SYSCALL_PUTCHAR, (uint32_t) &putchar_args, 0, 0);

                    // Re-print prompt
                    shell_print_prompt();
                }

                // Set shell row
                uint8_t row;
                syscall(SYSCALL_GET_CURSOR_ROW, (uint32_t) &row, 0, 0);
                shell_status.shell_row = row;

                // Reset shell input
                stringn_create(&shell_input);

            } else if (buf == '\b') {
                // Get current col
                int col;
                int row;
                syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &col, 0, 0);
                syscall(SYSCALL_GET_CURSOR_ROW, (uint32_t) &row, 0, 0);

                // Check if col is at del limit
                if (col > shell_status.del_limit || row > shell_status.shell_row) {
                    // Get rightmost character
                    char c = shell_input.buf[shell_input.len - 1];

                    // Implement backspace with ctrl
                    do {
                        // Print backspace to screen
                        syscall(SYSCALL_PUTCHAR, (uint32_t) &putchar_args, 0, 0);
                        syscall(SYSCALL_GET_CURSOR_ROW, (uint32_t) &row, 0, 0);
                        syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &col, 0, 0);

                        // Remove last character from shell input
                        shell_input.buf[shell_input.len - 1] = '\0';
                        if (shell_input.len > 0) shell_input.len--;

                        // Get new character
                        c = shell_input.buf[shell_input.len - 1];
                    } while (c != ' ' && (col > shell_status.del_limit || row > shell_status.shell_row) && press_ctrl);
                }
            } else if (buf != '\0') {
                // Print character to screen
                syscall(SYSCALL_PUTCHAR, (uint32_t) &putchar_args, 0, 0);

                // Append character to shell input
                stringn_appendchar(&shell_input, buf);
            }
        }
    }
    return 0;
}

void cd(struct StringN folder) {
    // Check if folder is in current directory
    struct FAT32DirectoryTable folder_table;

    struct FAT32DriverRequest request = {
        .buf = &folder_table,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = sizeof(struct FAT32DirectoryTable)
    };
    for (uint8_t i = 0; i < folder.len; i++) {
        request.name[i] = folder.buf[i];
    }

    int8_t retcode;
    syscall(SYSCALL_READ_DIRECTORY, (uint32_t) &request, (uint32_t) &retcode, 0);

    switch (retcode) {
        case 0:
            // Set current directory to folder
            currentDir = folder_table;
            set_current_cluster();
            break;
        case 1:
            struct SyscallPutsArgs args = {
                .buf = "Directory ",
                .count = strlen(args.buf),
                .fg_color = 0xC,
                .bg_color = 0x0
            };

            args.buf = "'";
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

            args.buf = folder.buf;
            args.count = strlen(args.buf);
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

            args.buf = "'";
            args.count = strlen(args.buf);
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

            args.buf = " is not a folder!";
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            break;
        case 2:
            struct SyscallPutsArgs args2 = {
                .buf = "Folder ",
                .count = strlen(args2.buf),
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

            args2.buf = "'";
            args2.count = strlen(args2.buf);
            args2.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

            args2.buf = folder.buf;
            args2.count = strlen(args2.buf);
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

            args2.buf = "'";
            args2.count = strlen(args2.buf);
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

            args2.buf = " was not found in current directory!";
            args2.count = strlen(args2.buf);
            args2.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);
            break;
        default:
            break;
    }
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