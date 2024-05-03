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

//Definisi Fungsi Command
//Definisi command mkdir (Make Directory)
void mkdir(struct StringN folder_Name);
//Definisi command cd (Change Directory)
void cd(struct StringN folder);
//Definisi command rm (remove)
void rm(struct StringN folder);
//Definisi command cat
void cat(struct StringN filename);

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

// ls command
void ls(){
    struct FAT32DirectoryTable table = currentDir;
    for (int i = 2; i < 64; i++) {
        // Check if entry is empty
        struct FAT32DirectoryEntry entry = table.table[i];
        if (entry.name[0] == '\0') {
            break;
        }

        // Print entry name
        struct SyscallPutsArgs args = {
            .buf = entry.name,
            .count = strlen(args.buf),
            .fg_color = 0x7,
            .bg_color = 0x0
        };

        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

        // Print space between entries
        struct SyscallPutsArgs args2 = {
            .buf = " ",
            .count = strlen(args2.buf),
            .fg_color = 0x7,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);
    }
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
        ls();
    } else if (strcmp(command, SHELL_MKDIR)) {
        mkdir(arg1);
    } else if (strcmp(command, SHELL_CAT)) {
        cat(arg1);
    } else if (strcmp(command, SHELL_CP)) {

    } else if (strcmp(command, SHELL_RM)) {
        rm(arg1);
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

void mkdir(struct StringN folder_Name){
    struct FAT32DriverRequest request = {
        .name = "\0\0\0\0\0\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = 0,
    };
    struct SyscallPutsArgs args = {
        .buf = "Directory ",
        .count = strlen(args.buf),
        .fg_color = 0xC,
        .bg_color = 0x0
    };
    if(folder_Name.len > 8){
        args.buf = "Directory name is too long! (Maximum 8 Characters)";
        args.count = strlen(args.buf);
        args.fg_color = 0xC;
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
    }
    else{
        for (uint8_t i = 0; i < folder_Name.len; i++) {
            request.name[i] = folder_Name.buf[i];
        }
        int8_t retcode;
        syscall(SYSCALL_WRITE, (uint32_t) &request, (uint32_t) &retcode, 0);
        switch (retcode) {
        case 0:
            args.buf = "Operation success! ";
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = "'";
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = folder_Name.buf;
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = "'";
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = "has been created..";
            args.count = strlen(args.buf);
            args.fg_color = 0xE;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            break;
        case 1:
            args.buf = "mkdir: cannot create directory ";
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = "'";
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = folder_Name.buf;
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = "'";
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            args.buf = ": File exists";
            args.count = strlen(args.buf);
            args.fg_color = 0xC;
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            break;
        default:
            break;
    }
}

    // memcpy(request.name, f, sizeof(request.name));
    // request.name[sizeof(request.name)-1] = '';
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

void rm(struct StringN folder){
    struct FAT32DriverRequest request = {
        .name = "\0\0\0\0\0\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = 0,
    };
    struct SyscallPutsArgs args = {
        .buf = "Directory ",
        .count = strlen(args.buf),
        .fg_color = 0xC,
        .bg_color = 0x0
    };
    if(folder.len > 8){
        args.buf = "rm: cannot remove : name is too long! (Maximum 8 Characters)";
        args.count = strlen(args.buf);
        args.fg_color = 0xC;
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
    }
    else{
       for (uint8_t i = 0; i < folder.len; i++) {
            request.name[i] = folder.buf[i];
        }
        int8_t retcode;
        syscall(SYSCALL_DELETE,(uint32_t) &request, (uint32_t) &retcode, 0);
        switch (retcode){
            case 0:
                args.buf = "Operation success! ";
                args.count = strlen(args.buf);
                args.fg_color = 0xE;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = "'";
                args.count = strlen(args.buf);
                args.fg_color = 0xE;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = folder.buf;
                args.count = strlen(args.buf);
                args.fg_color = 0xE;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = "'";
                args.count = strlen(args.buf);
                args.fg_color = 0xE;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = "has been removed..";
                args.count = strlen(args.buf);
                args.fg_color = 0xE;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                break;
            case 1:
                args.buf = "rm: cannot remove '";
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = folder.buf;
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = "': No such file or directory";
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                break;
        }
    }
}

// meong
void cat(struct StringN filename) {
    uint8_t buf[10 * CLUSTER_SIZE];

    struct FAT32DriverRequest request = {
        .buf = &buf,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = 10 * CLUSTER_SIZE
    };

    for (uint8_t i = 0; i < filename.len; i++) {
        request.name[i] = filename.buf[i];
    }

    int8_t retcode;
    syscall(SYSCALL_READ, (uint32_t) &request, (uint32_t) &retcode, 0);

    switch (retcode) {
        case 0:
            struct SyscallPutsArgs args = {
                .buf = request.buf,
                .count = strlen(request.buf),
                .fg_color = 0x7,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
            break;
        case 1:
            struct SyscallPutsArgs a_folder = {
                .buf = "cat: ",
                .count = strlen("cat: "),
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &a_folder, 0, 0);

            struct SyscallPutsArgs filename_args1 = {
                .buf = filename.buf,
                .count = filename.len,
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &filename_args1, 0, 0);

            struct SyscallPutsArgs a_folder2 = {
                .buf = ": Is a directory",
                .count = strlen(": Is a directory"),
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &a_folder2, 0, 0);
            break;
        case 2:
            struct SyscallPutsArgs not_found_args = {
                .buf = "cat: ",
                .count = strlen("cat: "),
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_found_args, 0, 0);

            struct SyscallPutsArgs filename_args2 = {
                .buf = filename.buf,
                .count = filename.len,
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &filename_args2, 0, 0);

            struct SyscallPutsArgs not_found_args2 = {
                .buf = ": No such file or directory",
                .count = strlen(": No such file or directory"),
                .fg_color = 0xC,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_found_args2, 0, 0);
            break;
        default:
            break;
    }
}