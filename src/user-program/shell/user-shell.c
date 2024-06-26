#include <stdint.h>
#include "../../header/filesystem/fat32.h"
#include "../SYSCALL_LIBRARY.h"
#include "../utils.h"
#include "shell-background.h"

// Itoa table
char* itoa[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49"
};

// Filesystem variables
struct FAT32DirectoryTable rootDir;
struct StringN rootDirPath;
uint32_t rootDirCluster;

struct FAT32DirectoryTable currentDir;
struct StringN currentDirPath;
uint32_t currentDirCluster;

// Shell properties
#define SHELL_WINDOW_UPPER_HEIGHT 2
#define SHELL_WINDOW_LOWER_HEIGHT 21
#define SHELL_WINDOW_LEFT_WIDTH 2
#define SHELL_WINDOW_RIGHT_WIDTH 77

// Shell (specification) commands
#define SHELL_CD "cd"
#define SHELL_LS "ls"
#define SHELL_MKDIR "mkdir"
#define SHELL_CAT "cat"
#define SHELL_CP "cp"
#define SHELL_RM "rm"
#define SHELL_MV "mv"
#define SHELL_FIND "find"

// Shell (additional commands)
#define SHELL_CLEAR "clear"
#define SHELL_OKEGAS "okegas"

// Shell (process) commands
#define SHELL_EXEC "exec"
#define SHELL_PS "ps"
#define SHELL_KILL "kill"

// Variable for storing cp destination cluster
uint32_t cp_dest_cluster;

// Noprint for cd
#define CD_NO_PRINT 0
#define CD_PRINT    1

// Shell commands definitions
void cd(struct StringN folder, uint8_t print);
void ls();
void mkdir(struct StringN folder_Name);
void rm(struct StringN folder);
void cat(struct StringN filename);
void cp(struct StringN src, struct StringN dest);
void mv(struct StringN src, struct StringN dest);
void find(struct StringN filename);

// Shell process commands definitions
void exec(struct StringN filename);
void ps();
void kill(struct StringN pid);

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
// Recursively create current directory path
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

// Create current directory path
void create_path() {
    struct FAT32DirectoryEntry curr_entry = currentDir.table[0];
    uint32_t cluster = (curr_entry.cluster_high << 16) | curr_entry.cluster_low;

    currentDirPath = create_path_recursive(cluster);
}

// Set current directory cluster
void set_current_cluster() {    
    // Set currentDirCluster
    struct FAT32DirectoryEntry curr_entry = currentDir.table[0];
    currentDirCluster = (curr_entry.cluster_high << 16) | curr_entry.cluster_low;
}

// Create shell background
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
    syscall(SYSCALL_SET_CURSOR, 10, 8, 0);
}

// Print shell prompt
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

// Handle shell command input
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
    for (j = 1; ((i + j) < input.len && input.buf[i + j] != ' '); j++) {
        stringn_appendchar(&arg1, input.buf[i + j]);
    }

    // Get 2nd argument
    struct StringN arg2;
    stringn_create(&arg2);

    uint32_t k;
    for (k = 1; ((i + j + k) < input.len && input.buf[i + j + k] != ' '); k++) {
        stringn_appendchar(&arg2, input.buf[i + j + k]);
    }
    
    char* command = arg0.buf;

    if (strcmp(command, SHELL_CD)) {
        cd(arg1, CD_PRINT);
    } else if (strcmp(command, SHELL_LS)) {
        ls();
    } else if (strcmp(command, SHELL_MKDIR)) {
        mkdir(arg1);
    } else if (strcmp(command, SHELL_CAT)) {
        cat(arg1);
    } else if (strcmp(command, SHELL_CP)) {
        cp_dest_cluster = currentDirCluster;
        cp(arg1, arg2);
    } else if (strcmp(command, SHELL_RM)) {
        rm(arg1);
    } else if (strcmp(command, SHELL_MV)) {
        mv(arg1, arg2);
    } else if (strcmp(command, SHELL_FIND)) {
        find(arg1);
    } else if (strcmp(command, SHELL_CLEAR)) {
        syscall(SYSCALL_CLEAR_SCREEN, 0, 0, 0);
        syscall(SYSCALL_SET_CURSOR, SHELL_WINDOW_UPPER_HEIGHT, SHELL_WINDOW_LEFT_WIDTH, 0);
        shell_print_prompt();
    } else if (strcmp(command, SHELL_OKEGAS)) {
        struct SyscallPutsArgs args = {
            .buf = "TABRAK-TABRAK MASUK\nRAPPER KAMPUNG TABRAK MASUK\nMESKI JAUH JARAK PANDANG\nCOBA SEDIKIT MENGAMUK\nKU CIPTAKAN LIRIK DAN BEAT\nSECEPAT KILAT TAPI TAK SEMPIT\nBERDIRI TEGAR WALAUPUN SULIT\nTRA MAMPU BERSAING SILAHKAN PAMIT\nOK GAS-OK GAS\nTAMBAH DUA TORANG GAS",
            .count = strlen(args.buf),
            .fg_color = 0xE,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
    } else if (strcmp(command, SHELL_PS)) {
        ps();
    } else if (strcmp(command, SHELL_EXEC)) {
        exec(arg1);
    } else if (strcmp(command, SHELL_KILL)) {
        kill(arg1);
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

    // Set shell border limits
    struct SyscallKeyboardBordersArgs borders = {
        .up = SHELL_WINDOW_UPPER_HEIGHT,
        .down = SHELL_WINDOW_LOWER_HEIGHT,
        .left = SHELL_WINDOW_LEFT_WIDTH,
        .right = SHELL_WINDOW_RIGHT_WIDTH
    };
    syscall(SYSCALL_SET_KEYBOARD_BORDERS, (uint32_t) &borders, 0, 0);

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
    create_path();

    rootDir = currentDir;
    rootDirPath = currentDirPath;
    rootDirCluster = currentDirCluster;

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

    shell_status.shell_row = SHELL_WINDOW_UPPER_HEIGHT;

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

                // Set kernel shell open variable
                syscall(SYSCALL_SET_IS_SHELL_OPEN, (uint32_t) shell_status.is_open, 0, 0);

                // Get border limits
                uint8_t up = SHELL_WINDOW_UPPER_HEIGHT;
                // uint8_t down = SHELL_WINDOW_LOWER_HEIGHT;
                uint8_t left = SHELL_WINDOW_LEFT_WIDTH;
                uint8_t right = SHELL_WINDOW_RIGHT_WIDTH;

                // Clear screen and print prompt
                syscall(SYSCALL_CLEAR_SCREEN, 0, 0, 0);
                syscall(SYSCALL_SET_CURSOR, up - 1, left, 0);

                // Print window header
                for (int j = left; j < right + 1; j++) {
                    struct SyscallPutsArgs args = {
                        .buf = " ",
                        .count = 1,
                        .fg_color = 0x0,
                        .bg_color = BIOS_DARK_GRAY
                    };

                    syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                }

                // Print window title
                struct SyscallPutsAtArgs title = {
                    .buf = "Shell",
                    .count = strlen(title.buf),
                    .fg_color = BIOS_WHITE,
                    .bg_color = BIOS_DARK_GRAY,
                    .row = up - 1,
                    .col = left + 3
                };
                syscall(SYSCALL_PUTS_AT, (uint32_t) &title, 0, 0);

                // Print window buttons
                struct SyscallPutsAtArgs x = {
                    .buf = " X ",
                    .count = 3,
                    .fg_color = BIOS_BLACK,
                    .bg_color = BIOS_RED,
                    .row = up - 1,
                    .col = right - 2
                };
                syscall(SYSCALL_PUTS_AT, (uint32_t) &x, 0, 0);

                struct SyscallPutsAtArgs O = {
                    .buf = " O ",
                    .count = 3,
                    .fg_color = BIOS_BLACK,
                    .bg_color = BIOS_YELLOW,
                    .row = up - 1,
                    .col = right - 5
                };
                syscall(SYSCALL_PUTS_AT, (uint32_t) &O, 0, 0);

                struct SyscallPutsAtArgs m = {
                    .buf = " _ ",
                    .count = 3,
                    .fg_color = BIOS_BLACK,
                    .bg_color = BIOS_LIGHT_GREEN,
                    .row = up - 1,
                    .col = right - 8
                };
                syscall(SYSCALL_PUTS_AT, (uint32_t) &m, 0, 0);

                // Print shell prompt
                shell_print_prompt();
            }
        } else {
            // Handler if user presses ctrl + s
            if (press_ctrl && buf == 's') {
                // Set shell to open
                shell_status.is_open = false;

                // Set kernel shell open variable
                syscall(SYSCALL_SET_IS_SHELL_OPEN, (uint32_t) shell_status.is_open, 0, 0);

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

// Shell command implementation
// cd
void cd(struct StringN folder, uint8_t print) {        
    bool isCwd = strcmp(folder.buf, ".");
    bool isRoot = strcmp(folder.buf, "..");
    if (isCwd || isRoot) {
        if (isRoot) {
            struct FAT32DirectoryEntry parent_entry = currentDir.table[1];
            uint32_t parent_cluster = (parent_entry.cluster_high << 16) | parent_entry.cluster_low;

            syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, parent_cluster, 0);
            set_current_cluster();

            create_path();
        }

        if (print == CD_PRINT) {
            // Print finishing prompt
            struct SyscallPutsArgs args = {
                .buf = "Current directory successfully changed to ",
                .count = strlen(args.buf),
                .fg_color = BIOS_LIGHT_GREEN,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

            struct SyscallPutsArgs args2 = {
                .buf = currentDir.table[0].name,
                .count = strlen(args2.buf),
                .fg_color = BIOS_YELLOW,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

            struct SyscallPutsArgs args3 = {
                .buf = "!",
                .count = strlen(args3.buf),
                .fg_color = BIOS_LIGHT_GREEN,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args3, 0, 0);
        }
        return;
    } else {
        struct FAT32DirectoryTable table = currentDir;
        for (int i = 2; i < 64; i++) {
            struct FAT32DirectoryEntry entry = table.table[i];
            // if (entry.name[0] == '\0') {
            //     break;
            // }
            if (strcmp(entry.name, folder.buf) == true && strcmp(entry.ext, "\0\0\0") == true) {
                // If entry is a file, return
                if (entry.attribute != ATTR_SUBDIRECTORY) {
                    struct SyscallPutsArgs args = {
                        .buf = "cd error: Not a directory!",
                        .count = strlen(args.buf),
                        .fg_color = BIOS_LIGHT_RED,
                        .bg_color = 0x0
                    };
                    syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                    return;
                }

                // Construct FAT32DriverRequest
                struct FAT32DriverRequest request = {
                    .buf = &currentDir,
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentDirCluster,
                    .buffer_size = sizeof(struct FAT32DirectoryTable)
                    {}
                };

                // Set request name
                for (uint8_t i = 0; i < folder.len; i++) {
                    request.name[i] = folder.buf[i];
                }

                // Read new directory to currentDir
                uint32_t retcode;
                syscall(SYSCALL_READ_DIRECTORY, (uint32_t) &request, (uint32_t) &retcode, 0);

                retcode++;

                // Set new current cluster
                set_current_cluster();
                create_path();

                if (print == CD_PRINT) {                    
                    // Print finishing prompt
                    struct SyscallPutsArgs args = {
                        .buf = "Current directory successfully changed to ",
                        .count = strlen(args.buf),
                        .fg_color = BIOS_LIGHT_GREEN,
                        .bg_color = 0x0
                    };
                    syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

                    struct SyscallPutsArgs args2 = {
                        .buf = request.buf,
                        .count = strlen(args2.buf),
                        .fg_color = BIOS_YELLOW,
                        .bg_color = 0x0
                    };
                    syscall(SYSCALL_PUTS, (uint32_t) &args2, 0, 0);

                    struct SyscallPutsArgs args3 = {
                        .buf = "!",
                        .count = strlen(args3.buf),
                        .fg_color = BIOS_LIGHT_GREEN,
                        .bg_color = 0x0
                    };
                    syscall(SYSCALL_PUTS, (uint32_t) &args3, 0, 0);
                }
                return;
            }
        }

        if (print == CD_PRINT) {
            struct SyscallPutsArgs args = {
                .buf = "cd error: Directory not found!",
                .count = strlen(args.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
        }
    }
}

// ls
void ls(){
    struct FAT32DirectoryTable table = currentDir;
    // Print folder as yellow
    struct SyscallPutsArgs fold = {
        .buf = "[folder]",
        .count = strlen(fold.buf),
        .fg_color = BIOS_YELLOW,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &fold, 0, 0);

    // Print space between folder and file
    struct SyscallPutsArgs space = {
        .buf = " ",
        .count = strlen(space.buf),
        .fg_color = 0x7,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &space, 0, 0);

    // Print file as light cyan
    struct SyscallPutsArgs file = {
        .buf = "[file]\n",
        .count = strlen(file.buf),
        .fg_color = BIOS_LIGHT_CYAN,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &file, 0, 0);

    // Print current directory
    struct SyscallPutsArgs cwd = {
        .buf = ". ",
        .count = strlen(cwd.buf),
        .fg_color = BIOS_YELLOW,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &cwd, 0, 0);

    // Print parent directory
    struct SyscallPutsArgs pd = {
        .buf = ".. ",
        .count = strlen(pd.buf),
        .fg_color = BIOS_YELLOW,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &pd, 0, 0);

    // Print file as blue
    for (int i = 2; i < 64; i++) {
        // Check if entry is empty
        struct FAT32DirectoryEntry entry = table.table[i];
        if (entry.user_attribute != UATTR_NOT_EMPTY) continue;

        // Print entry name
        struct SyscallPutsArgs args = {
            .buf = entry.name,
            .count = strlen(args.buf),
            .fg_color = entry.attribute == ATTR_SUBDIRECTORY ? BIOS_YELLOW : BIOS_LIGHT_CYAN,
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

    // Print newline
    struct SyscallPutsArgs newline = {
        .buf = "\n",
        .count = strlen(newline.buf),
        .fg_color = 0x7,
        .bg_color = 0x0
    };
}

// mkdir
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
        args.buf = "Directory name is too long! Maximum name length is 8 characters.";
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
            args.buf = " has been created.";
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

    // Update current directory table
    syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);
}

// rm
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
        args.buf = "rm: cannot remove: Name is too long! Maximum name length is 8 characters.";
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
                args.buf = " has been removed.";
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

            case 2:
                args.buf = "rm: cannot remove '";
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = folder.buf;
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                args.buf = "': Directory is not empty";
                args.count = strlen(args.buf);
                args.fg_color = 0xC;
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
                break;
        }
    }

    // Update current directory table
    syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);
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

// cp
void cp(struct StringN src, struct StringN dest) {
    // If source or destination name is too long, return error
    if (src.len > 8 || dest.len > 8) {
        struct SyscallPutsArgs args = {
            .buf = "cp: cannot copy: Name is too long! Maximum name length is 8 characters.",
            .count = strlen(args.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
        return;
    }

    // Return error if not enough arguments
    if (src.len == 0 || dest.len == 0) {
        struct SyscallPutsArgs args = {
            .buf = "cp error: Not enough arguments! Usage: cp <source> <destination>",
            .count = strlen(args.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
        return;
    }

    // Source request
    uint8_t src_buf[15 * CLUSTER_SIZE];
    struct FAT32DriverRequest src_req = {
        .buf = &src_buf,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = 15 * CLUSTER_SIZE
    };

    for (uint8_t i = 0; i < src.len; i++) {
        src_req.name[i] = src.buf[i];
    }

    // Read source
    int8_t retcode;
    syscall(SYSCALL_READ, (uint32_t) &src_req, (uint32_t) &retcode, 0);

    // If source file not found, return error
    switch (retcode) {
        case 1:
            struct SyscallPutsArgs not_file = {
                .buf = "cp error: Source is not a file!",
                .count = strlen(not_file.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_file, 0, 0);
            return;
        case 2:
            struct SyscallPutsArgs not_found = {
                .buf = "cp error: Source file is not a found in current directory!",
                .count = strlen(not_found.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_found, 0, 0);
            return;
        case -1:
            struct SyscallPutsArgs not_enough_size = {
                .buf = "cp error: Source file is too large!",
                .count = strlen(not_found.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_enough_size, 0, 0);
            return;
        default:
            break;
    }

    // Count filesize
    uint32_t filesize = 0;
    uint8_t* src_buf_ptr = (uint8_t*) src_req.buf;
    uint32_t i = 0;

    uint32_t blank_count = 0;
    while (true) {

        if (src_buf_ptr[i] != '\0') {
            filesize++;
            i = filesize * CLUSTER_SIZE;
            i++;
        } else if (blank_count == 10) {
            break;
        } else {
            blank_count++;
        }
    }

    // Modify src_req for writing
    src_req.buffer_size = filesize * CLUSTER_SIZE;

    for (uint8_t i = 0; i < dest.len; i++) {
        src_req.name[i] = dest.buf[i];
    }

    // Write file
    src_req.parent_cluster_number = cp_dest_cluster;
    syscall(SYSCALL_WRITE, (uint32_t) &src_req, (uint32_t) &retcode, 0);

    // Return error if file already exists
    if (retcode == 1) {
        struct SyscallPutsArgs file_exists = {
            .buf = "cp error: File with the same name already exists in current directory!",
            .count = strlen(file_exists.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &file_exists, 0, 0);
        return;
    }

    // Update current directory table
    syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);

    // Print success message
    if (cp_dest_cluster != currentDirCluster) return;
    struct SyscallPutsArgs success = {
        .buf = "cp: File successfully copied to ",
        .count = strlen(success.buf),
        .fg_color = BIOS_LIGHT_GREEN,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &success, 0, 0);

    struct SyscallPutsArgs dest_name = {
        .buf = dest.buf,
        .count = dest.len,
        .fg_color = BIOS_YELLOW,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &dest_name, 0, 0);

    struct SyscallPutsArgs success2 = {
        .buf = "!",
        .count = strlen(success2.buf),
        .fg_color = BIOS_LIGHT_GREEN,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &success2, 0, 0);
}

// to not found --> rename
// to folder --> move
// to file --> error

// mv
void mv(struct StringN src, struct StringN dest) {
    // If source or destination name is too long, return error
    if (src.len > 8 || dest.len > 8) {
        struct SyscallPutsArgs args = {
            .buf = "mv: cannot move: Name is too long! Maximum name length is 8 characters.",
            .count = strlen(args.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
        return;
    }

    // Return error if not enough arguments
    if (src.len == 0 || dest.len == 0) {
        struct SyscallPutsArgs args = {
            .buf = "mv error: Not enough arguments! Usage: mv <source> <destination>",
            .count = strlen(args.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);
        return;
    }

    // Check if src exists
    bool src_is_folder = false;
    struct FAT32DirectoryTable src_table;
    struct FAT32DriverRequest src_req = {
        .buf = &src_table,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = CLUSTER_SIZE
    };

    for (uint8_t i = 0; i < src.len; i++) {
        src_req.name[i] = src.buf[i];
    }

    int8_t retcode_src;
    syscall(SYSCALL_READ_DIRECTORY, (uint32_t) &src_req, (uint32_t) &retcode_src, 0);

    switch (retcode_src) {
        case 0:
            src_is_folder = true;
            break;
        case 2:
            struct SyscallPutsArgs not_found = {
                .buf = "mv error: Source file/folder not found in current directory!",
                .count = strlen(not_found.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_found, 0, 0);
            return;
        default:
            break;
    }

    // Check dest is file or folder
    struct FAT32DirectoryTable dest_table;
    struct FAT32DriverRequest dest_req = {
        .buf = &dest_table,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = currentDirCluster,
        .buffer_size = CLUSTER_SIZE
    };

    for (uint8_t i = 0; i < dest.len; i++) {
        dest_req.name[i] = dest.buf[i];
    }

    bool is_rename = false;
    int8_t retcode_dest;
    syscall(SYSCALL_READ_DIRECTORY, (uint32_t) &dest_req, (uint32_t) &retcode_dest, 0);

    switch (retcode_dest) {
        case 1:
            struct SyscallPutsArgs not_folder = {
                .buf = "mv error: Destination is not a folder!",
                .count = strlen(not_folder.buf),
                .fg_color = BIOS_LIGHT_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &not_folder, 0, 0);
            return;
        case 2:
            is_rename = true;
            break;
        default:
            break;
    }

    // Rename or move
    if (is_rename) {
        for (uint8_t i = 2; i < 64; i++) {
            struct FAT32DirectoryEntry* entry = &currentDir.table[i];
            if (strcmp(entry->name, src.buf) == true) {
                // Rename entry
                for (uint8_t j = 0; j < 8; j++) {
                    entry->name[j] = dest.buf[j];
                }

                // Write current dir to filesystem
                syscall(SYSCALL_WRITE_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);

                // Rename entry in src directory table
                if (src_is_folder) {
                    // Get src cluster
                    uint32_t src_cluster = (src_table.table[0].cluster_high << 16) | (src_table.table[0].cluster_low);

                    // Change name of src table
                    for (uint8_t j = 0; j < 8; j++) {
                        src_table.table[0].name[j] = dest.buf[j];
                    }

                    // Write src table to filesystem
                    syscall(SYSCALL_WRITE_CLUSTER, (uint32_t) &src_table, src_cluster, 0);
                }

                // Print success prompt
                struct SyscallPutsArgs success = {
                    .buf = "mv: File/folder successfully renamed to ",
                    .count = strlen(success.buf),
                    .fg_color = BIOS_LIGHT_GREEN,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &success, 0, 0);

                struct SyscallPutsArgs dest_name = {
                    .buf = dest.buf,
                    .count = dest.len,
                    .fg_color = BIOS_YELLOW,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &dest_name, 0, 0);

                struct SyscallPutsArgs success2 = {
                    .buf = "!",
                    .count = strlen(success2.buf),
                    .fg_color = BIOS_LIGHT_GREEN,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &success2, 0, 0);

                // Update current directory table
                syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);
                return;
            }
        }
    } else {
        // Find source
        struct FAT32DirectoryEntry src_entry;
        for (uint8_t i = 2; i < 64; i++) {
            src_entry = currentDir.table[i];

            if (strcmp(src_entry.name, src.buf) == true) {
                // Create an empty directory entry
                currentDir.table[i] = (struct FAT32DirectoryEntry) {0};
                break;
            }
        }

        // Write current dir to filesystem
        syscall(SYSCALL_WRITE_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);

        // Get destination folder cluster number
        uint32_t dest_cluster = (dest_table.table[0].cluster_high << 16) | dest_table.table[0].cluster_low;

        // Find destination
        struct FAT32DirectoryEntry dest_entry;
        for (uint8_t i = 2; i < 64; i++) {
            dest_entry = dest_table.table[i];

            if (dest_entry.user_attribute != UATTR_NOT_EMPTY) {
                // Copy source to destination
                dest_table.table[i] = src_entry;
                break;
            }
        }

        // Write destination dir to filesystem
        syscall(SYSCALL_WRITE_CLUSTER, (uint32_t) &dest_table, dest_cluster, 0);

        // Change src root if src is a folder
        if (src_is_folder) {
            // Get src cluster
            uint32_t src_cluster = (src_table.table[0].cluster_high << 16) | (src_table.table[0].cluster_low);

            // Change root of src table
            src_table.table[1].cluster_high = dest_table.table[0].cluster_high;
            src_table.table[1].cluster_low = dest_table.table[0].cluster_low;

            // Write src table to filesystem
            syscall(SYSCALL_WRITE_CLUSTER, (uint32_t) &src_table, src_cluster, 0);
        }

        // Print success prompt
        struct SyscallPutsArgs success = {
            .buf = "mv: File/folder successfully moved to ",
            .count = strlen(success.buf),
            .fg_color = BIOS_LIGHT_GREEN,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &success, 0, 0);

        struct SyscallPutsArgs dest_name = {
            .buf = dest.buf,
            .count = dest.len,
            .fg_color = BIOS_YELLOW,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &dest_name, 0, 0);

        struct SyscallPutsArgs success2 = {
            .buf = "!",
            .count = strlen(success2.buf),
            .fg_color = BIOS_LIGHT_GREEN,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &success2, 0, 0);

        // Update current directory table
        syscall(SYSCALL_READ_CLUSTER, (uint32_t) &currentDir, currentDirCluster, 0);
    } 
}

// find
void find_recursive(struct StringN filename, bool* found) {
    if (*found == true) return;

    for (uint8_t i = 2; i < 64; i++) {
        struct FAT32DirectoryEntry entry = currentDir.table[i];
        
        // Skip if entry is empty
        if (entry.user_attribute != UATTR_NOT_EMPTY) continue;

        // If entryname is equal to filename, end search
        if (strcmp(entry.name, filename.buf) == true) {
            if (!(*found)) {
                struct SyscallPutsArgs args = {
                    .buf = "File/folder found at ",
                    .count = strlen(args.buf),
                    .fg_color = BIOS_LIGHT_GREEN,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &args, 0, 0);

                struct StringN path;
                stringn_create(&path);
                stringn_appendstr(&path, currentDirPath.buf);
                stringn_appendstr(&path, entry.name);

                struct SyscallPutsArgs path_struct = {
                    .buf = path.buf,
                    .count = path.len,
                    .fg_color = BIOS_YELLOW,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &path_struct, 0, 0);

                struct SyscallPutsArgs excl = {
                    .buf = "!",
                    .count = strlen(excl.buf),
                    .fg_color = 0x7,
                    .bg_color = 0x0
                };
                syscall(SYSCALL_PUTS, (uint32_t) &excl, 0, 0);
            }

            *found = true;
            return;
        }

        // If entry is a folder, recursively find
        if (entry.attribute == ATTR_SUBDIRECTORY) {
            // Get folder name
            struct StringN folder_name;
            stringn_create(&folder_name);
            stringn_appendstr(&folder_name, entry.name);

            // Change directory
            cd(folder_name, CD_NO_PRINT);

            // Recursive find
            find_recursive(filename, found);
        }
    }
    struct StringN root;
    stringn_create(&root);
    stringn_appendstr(&root, "..");
    cd(root, CD_NO_PRINT);
}

void find(struct StringN filename) {
    struct FAT32DirectoryTable old_currentDir = currentDir;
    struct StringN old_currentDirPath = currentDirPath;
    uint32_t old_currentDirCluster = currentDirCluster;

    currentDir = rootDir;
    currentDirPath = rootDirPath;
    currentDirCluster = rootDirCluster;

    bool found = false;
    find_recursive(filename, &found);

    currentDir = old_currentDir;
    currentDirPath = old_currentDirPath;
    currentDirCluster = old_currentDirCluster;

    if (!found) {
        struct SyscallPutsArgs not_found = {
            .buf = "find: File/folder not found in filesystem!",
            .count = strlen(not_found.buf),
            .fg_color = BIOS_LIGHT_RED,
            .bg_color = 0x0
        };
        syscall(SYSCALL_PUTS, (uint32_t) &not_found, 0, 0);
    
    }
}

// exec
void exec(struct StringN filename) {
    // Construct FAT32DriverRequest
    struct FAT32DriverRequest request = {
        .name = "\0\0\0\0\0\0\0\0",
        .parent_cluster_number = 5,
        .buffer_size = 0x100000,
    };

    // Fill request name
    for (uint8_t i = 0; i < filename.len; i++) {
        request.name[i] = filename.buf[i];
    }

    // Create process
    int8_t retcode;
    syscall(SYSCALL_CREATE_PROCESS, (uint32_t) &request, (uint32_t) &retcode, 0);

    // Handle return code
    switch (retcode) {
        case 0:
            struct SyscallPutsArgs args_0 = {
                .buf = "Process created successfully!",
                .count = strlen(args_0.buf),
                .fg_color = BIOS_YELLOW,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args_0, 0, 0);
            break;
        case 1:
            struct SyscallPutsArgs args_1 = {
                .buf = "Exec error: Max process count reached!",
                .count = strlen(args_1.buf),
                .fg_color = BIOS_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args_1, 0, 0);
            break;

        case 2:
            struct SyscallPutsArgs args_2 = {
                .buf = "Exec error: Invalid entrypoint!",
                .count = strlen(args_2.buf),
                .fg_color = BIOS_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args_2, 0, 0);
            break;

        case 3:
            struct SyscallPutsArgs args_3 = {
                .buf = "Exec error: Not enough memory!",
                .count = strlen(args_3.buf),
                .fg_color = BIOS_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args_3, 0, 0);
            break;

        case 4:
            struct SyscallPutsArgs args_4 = {
                .buf = "Exec error: File not found in bin folder!",
                .count = strlen(args_4.buf),
                .fg_color = BIOS_RED,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &args_4, 0, 0);
            break;
    }
}

// ps
void ps() {
    // Get max process count
    uint32_t max_process_count;
    syscall(SYSCALL_GET_MAX_PROCESS_COUNT, (uint32_t) &max_process_count, 0, 0);

    // Newline struct
    struct SyscallPutsArgs newline = {
        .buf = "\n",
        .count = strlen(newline.buf),
        .fg_color = 0x0,
        .bg_color = 0x0
    };

    // Whitespace struct
    struct SyscallPutsArgs whitespace = {
        .buf = " ",
        .count = strlen(whitespace.buf),
        .fg_color = 0x0,
        .bg_color = 0x0
    };
    uint32_t COL_NAME = 11;
    uint32_t COL_STATE = 23;
    uint32_t COL_FRAME_COUNT = 36;

    // Print header
    struct SyscallPutsArgs header = {
        .buf = "PID      Name        State        Frame Count",
        .count = strlen(header.buf),
        .fg_color = BIOS_YELLOW,
        .bg_color = 0x0
    };
    syscall(SYSCALL_PUTS, (uint32_t) &header, 0, 0);

    // Print process info
    uint32_t col;
    for (uint32_t i = 0; i < max_process_count; i++) {
        // Create process_info struct
        struct SyscallProcessInfoArgs process_info = {
            .pid = i,
        };

        // Read i-th process info
        syscall(SYSCALL_GET_PROCESS_INFO, (uint32_t) &process_info, 0, 0);

        // Print process info if exists at i-th slot
        if (process_info.process_exists) {
            // Print newline
            syscall(SYSCALL_PUTS, (uint32_t) &newline, 0, 0);

            // PID
            struct SyscallPutsArgs pid = {
                .buf = itoa[process_info.pid],
                .count = strlen(pid.buf),
                .fg_color = BIOS_WHITE,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &pid, 0, 0);

            do {
                syscall(SYSCALL_PUTS, (uint32_t) &whitespace, 0, 0);
                syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &col, 0, 0);
            } while (col < COL_NAME);

            // Process name
            struct SyscallPutsArgs name = {
                .buf = process_info.name,
                .count = strlen(name.buf),
                .fg_color = BIOS_LIGHT_GREEN,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &name, 0, 0);

            do {
                syscall(SYSCALL_PUTS, (uint32_t) &whitespace, 0, 0);
                syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &col, 0, 0);
            } while (col < COL_STATE);

            // Process state
            struct SyscallPutsArgs state = {
                .buf = process_info.state,
                .count = strlen(state.buf),
                .fg_color = BIOS_LIGHT_CYAN,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &state, 0, 0);

            do {
                syscall(SYSCALL_PUTS, (uint32_t) &whitespace, 0, 0);
                syscall(SYSCALL_GET_CURSOR_COL, (uint32_t) &col, 0, 0);
            } while (col < COL_FRAME_COUNT);

            // Process frame count
            struct SyscallPutsArgs frame_count = {
                .buf = itoa[process_info.page_frame_used_count],
                .count = strlen(frame_count.buf),
                .fg_color = BIOS_LIGHT_BLUE,
                .bg_color = 0x0
            };
            syscall(SYSCALL_PUTS, (uint32_t) &frame_count, 0, 0);
        }
    }
}

void kill(struct StringN pid_string) {
    if (strcmp(pid_string.buf, "0")) syscall(SYSCALL_KILL_PROCESS, 0, 0, 0);
    if (strcmp(pid_string.buf, "1")) syscall(SYSCALL_KILL_PROCESS, 1, 0, 0);
    if (strcmp(pid_string.buf, "2")) syscall(SYSCALL_KILL_PROCESS, 2, 0, 0);
    if (strcmp(pid_string.buf, "3")) syscall(SYSCALL_KILL_PROCESS, 3, 0, 0);
    if (strcmp(pid_string.buf, "4")) syscall(SYSCALL_KILL_PROCESS, 4, 0, 0);
    if (strcmp(pid_string.buf, "5")) syscall(SYSCALL_KILL_PROCESS, 5, 0, 0);
    if (strcmp(pid_string.buf, "6")) syscall(SYSCALL_KILL_PROCESS, 6, 0, 0);
    if (strcmp(pid_string.buf, "7")) syscall(SYSCALL_KILL_PROCESS, 7, 0, 0);
    if (strcmp(pid_string.buf, "8")) syscall(SYSCALL_KILL_PROCESS, 8, 0, 0);
    if (strcmp(pid_string.buf, "9")) syscall(SYSCALL_KILL_PROCESS, 9, 0, 0);
}