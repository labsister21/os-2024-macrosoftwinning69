#include "header/interrupt/interrupt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/cpu/gdt.h"
#include "header/filesystem/fat32.h"
#include "header/text/framebuffer.h"
#include "header/kernel-entrypoint.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"
#include "header/clock/cmos.h"
#include "user-program/SYSCALL_LIBRARY.h"

#include "user-program/utils.h"

/**
 * @brief Function to initiate the search for files or directories in a FAT32 file system.
 * 
 * This function initiates the search for files or directories in a FAT32 file system starting
 * from the root directory.
 * 
 * @param request The FAT32 driver request structure containing search parameters.
 * @return Returns an integer representing the result of the search operation.
 */
int8_t find_start(struct FAT32DriverRequest request);

/**
 * @brief Recursive function to search for files or directories in a FAT32 file system.
 * 
 * This function recursively searches for files or directories in a FAT32 file system starting
 * from a specified directory cluster.
 * 
 * @param request The FAT32 driver request structure containing search parameters.
 * @param path A pointer to a character array to construct the path of the searched files or directories.
 * @return Returns an integer representing the result of the search operation.
 */
int8_t find(struct FAT32DriverRequest request, char* path);

/**
 * Add source data to destination data
 * 
 * @param dest Pointer to destination memory
 * @param src Pointer to source memory
 * @param dest_size Destination memory size in byte
 * @param src_size Source memory size in byte
*/
void* memadd(void* restrict dest, const void* restrict src, size_t size1, size_t size2);

/**
 * Append string to buffer
 * 
 * @param buffer Pointer to buffer
 * @param str Pointer to string
*/
void append_to_buffer(void *buffer, const char *str);

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8) out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

void puts(struct SyscallPutsArgs args);

void main_interrupt_handler(struct InterruptFrame frame) {
    switch (frame.int_number) {
        case (PIC1_OFFSET + IRQ_TIMER):
            // Construct context to save to PCB
            struct Context ctx = {
                .cpu = frame.cpu,
                .eip = frame.int_stack.eip,
                .eflags = frame.int_stack.eflags
            };
            
            // Save context to current running process
            scheduler_save_context_to_current_running_pcb(ctx);

            // Send PIC_ACK 
            pic_ack(IRQ_TIMER);

            // Run scheduler process switch 
            scheduler_switch_to_next_process();
            break;
        case (PIC1_OFFSET + IRQ_KEYBOARD):
            keyboard_isr();
            break;
        case (0x30):
            syscall(frame);
            break;
    }
}

void puts(struct SyscallPutsArgs args) {
    // Get arguments
    char* buf = args.buf;
    uint32_t count = args.count;
    uint32_t fg_color = args.fg_color;
    uint32_t bg_color = args.bg_color;
    
    while (count && *buf != '\0') {
        if (*buf == '\n') {     // enter
            // Handler untuk di baris terakhir
            if (keyboard_state.row == keyboard_state.down_limit) {
                // Set limits
                uint8_t up = keyboard_state.up_limit;
                uint8_t down = keyboard_state.down_limit;
                uint8_t left = keyboard_state.left_limit;
                uint8_t right = keyboard_state.right_limit;

                // Scroll up
                for (int i = up; i < down; i++) {
                    for (int j = left; j < right + 1; j++) {
                        char c = FRAMEBUFFER_MEMORY_OFFSET[((i + 1) * 80 + j) * 2];
                        char fg = FRAMEBUFFER_MEMORY_OFFSET[((i + 1) * 80 + j) * 2 + 1] & 0x0F;
                        char bg = FRAMEBUFFER_MEMORY_OFFSET[((i + 1) * 80 + j) * 2 + 1] >> 4;

                        framebuffer_write(i, j, c, fg, bg);
                    }
                }
                for (int j = left; j < right + 1; j++) {
                    framebuffer_write(down, j, ' ', 0x07, 0x00);
                }
            } else {
                // maju ke baris berikutnya
                keyboard_state.row++;
            }

            // Reset col
            keyboard_state.col = keyboard_state.left_limit;

        } else if (*buf == '\b') {  // backspace
            // hapus karakter sebelumnya jika buffer tidak kosong
            if (keyboard_state.col > keyboard_state.left_limit) {
                keyboard_state.col--;
                framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0x07, 0x00);
            } else if (keyboard_state.row > keyboard_state.up_limit) { // jika posisi kolom adalah 0
                // kembali ke baris sebelumnya dan ke kolom terakhir yang berisi karakter non-spasi
                keyboard_state.row--;
                keyboard_state.col = keyboard_state.last_non_space_col[keyboard_state.row] + 1;
            }
        } else if (*buf == '\t') {  // tab
            // maju ke kolom berikutnya yang merupakan kelipatan 4
            keyboard_state.col = (keyboard_state.col + 4) & ~3;
        } else {
            // menyimpan karakter ascii ke dalam framebuffer
            framebuffer_write(keyboard_state.row, keyboard_state.col, *buf, fg_color, bg_color);
            keyboard_state.last_non_space_col[keyboard_state.row] = keyboard_state.col;
            
            // Update col
            if (!(keyboard_state.row == keyboard_state.down_limit && keyboard_state.col == keyboard_state.right_limit)) {
                if (keyboard_state.col == keyboard_state.right_limit) {
                    keyboard_state.row++;
                    keyboard_state.col = keyboard_state.left_limit;
                } else {
                    keyboard_state.col++;
                }
            }
            // jika karakter bukan spasi, perbarui posisi kolom terakhir yang berisi karakter non-spasi
            // if (*buf != ' ') {
            // }
        }
        buf++;
        count--;
    }

    framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
}

void puts_at(struct SyscallPutsAtArgs args) {
    // Get arguments
    char* buf = args.buf;
    uint32_t count = args.count;
    uint32_t fg_color = args.fg_color;
    uint32_t bg_color = args.bg_color;
    uint8_t row = args.row;
    uint8_t col = args.col;

    // Print to screen
    while (count && *buf != '\0') {
        framebuffer_write(row, col, *buf, fg_color, bg_color);

        buf++;
        col++;
        count--;
    }
}

int8_t find_start(struct FAT32DriverRequest request){
    char dot = '.';
    char path[512];
    memset(path, 0, 512);
    memcpy(path,&dot,1);
    int8_t path_length = find (request, path);

    if(path_length == 0){
        return 1;
    }
    return 0;
}

int8_t find(struct FAT32DriverRequest request, char* path){
    int8_t path_length = 0;

    char newline = '\n';
    char slash = '/';

    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, request.parent_cluster_number, 1);

    memadd(path, &slash,strlen(path),1);
    memadd(path, dir_table.table[0].name, strlen(path),8);

    for(unsigned int i = 2; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++){
        if(memcmp(dir_table.table[i].name, request.name, 8) == 0){
           path_length++;
           if(dir_table.table[i].attribute == ATTR_SUBDIRECTORY){
                memadd(path,&slash,strlen(path),1);
                memadd(path,dir_table.table[i].name, strlen(path),8);
                path[strlen(path)] = '\0';

                append_to_buffer(request.buf,path);
                append_to_buffer(request.buf,&newline);
                return path_length;
           }
           else{
                char TempPath[512];
                memset(TempPath, 0, 512);
                memcpy(TempPath,path,strlen(path));

                char slash = '/';
                memadd(TempPath, &slash, strlen(TempPath),1);
                memadd(TempPath, dir_table.table[i].name, strlen(TempPath),8);
                TempPath[strlen(TempPath)] = '\0';

                if(dir_table.table[i].ext[0]  != 0x0){
                    char dot = '.';
                    memadd(TempPath, &dot, strlen(TempPath),1);
                    memadd(TempPath, dir_table.table[i].ext, strlen(TempPath),3);
                }
                append_to_buffer(request.buf,TempPath);
                append_to_buffer(request.buf,&newline);
                // return path_length;                
           }
        }

        if(dir_table.table[i].attribute == ATTR_SUBDIRECTORY && dir_table.table[i].name[0] != (uint8_t) 0x0) {
            struct FAT32DriverRequest new_request = {
                .parent_cluster_number = dir_table.table[i].cluster_low,
                .buffer_size = request.buffer_size,
                .buf = request.buf,
            };

            memcpy(new_request.name, request.name, 8);
            memcpy(new_request.ext, request.ext, 3);

            char TempPath[512];
            memset(TempPath, 0, 512);
            memcpy(TempPath,path,strlen(path));

            int8_t code = find(new_request,TempPath);
            path_length += code;
            memcpy(request.buf, new_request.buf, request.buffer_size);
        }
    }
    return path_length;
}

void* memadd(void* restrict dest, const void* restrict src, size_t size1, size_t size2){
    uint8_t *destbuf = (uint8_t*) dest;
    memcpy(destbuf,dest,size1);
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < size2; i++)
    {
        destbuf[i+size1] = srcbuf[i];
    }
    return destbuf;
}

// Function to append a string to a buffer
void append_to_buffer(void *buffer, const char *str) {
    char *buf = (char *)buffer;
    while (*buf != '\0') {
        buf++;
    }
    while (*str != '\0') {
        *buf++ = *str++;
    }
    *buf = '\0';
}

void get_process_info(struct SyscallProcessInfoArgs* args) {
    // Get process index
    uint32_t pid = args->pid;

    // Set process_exists flag
    args->process_exists = process_manager_state.process_list_used[pid];

    // If process exists, get the process info
    if (args->process_exists) {
        // Get process metadata
        struct ProcessControlBlock* pcb = &(_process_list[pid]);
        
        // Process name
        for (uint8_t i = 0; i < PROCESS_NAME_LENGTH_MAX; i++) {
            args->name[i] = pcb->metadata.name[i];
        }
        
        // Process state
        switch (pcb->metadata.state) {
            case PROCESS_STATE_RUNNING:
                for (uint8_t i = 0; i < 7; i++) {
                    args->state[i] = "RUNNING"[i];
                }
                break;
            case PROCESS_STATE_READY:
                for (uint8_t i = 0; i < 5; i++) {
                    args->state[i] = "READY"[i];
                }
                break;
            case PROCESS_STATE_BLOCKED:
                for (uint8_t i = 0; i < 7; i++) {
                    args->state[i] = "BLOCKED"[i];
                }
                break;
        } 

        // Get process memory info
        args->page_frame_used_count = pcb->memory.page_frame_used_count;
    }
}

struct {
    bool shell_open;
} user_variables = {
    .shell_open = false,
};

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        // SYSCALL 0
        case SYSCALL_READ: 
        ;
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 1
        case SYSCALL_READ_DIRECTORY: 
        ;
            *((int8_t*) frame.cpu.general.ecx) = read_directory(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 2
        case SYSCALL_WRITE: 
        ;
            *((int8_t*) frame.cpu.general.ecx) = write(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 3
        case SYSCALL_DELETE: 
        ;
            *((int8_t*) frame.cpu.general.ecx) = delete(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 4
        case SYSCALL_GETCHAR: 
        ;
            // keyboard_state_activate();

            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;

        // SYSCALL 5
        case SYSCALL_PUTCHAR: 
        ;
            struct SyscallPutsArgs* pointer = (struct SyscallPutsArgs*) frame.cpu.general.ebx;
            pointer->count = 1;

            puts(*pointer);
            break;

        // SYSCALL 6
        case SYSCALL_PUTS: 
        ;
            struct SyscallPutsArgs* pointer_puts = (struct SyscallPutsArgs*) frame.cpu.general.ebx;

            puts(*pointer_puts);
            break;

        // SYSCALL 7
        case SYSCALL_PUTS_AT: 
        ;
            struct SyscallPutsAtArgs* pointer_puts_at = (struct SyscallPutsAtArgs*) frame.cpu.general.ebx;

            puts_at(*pointer_puts_at);
            break;

        // SYSCALL 8
        case SYSCALL_ACTIVATE_KEYBOARD: 
        ;
            keyboard_state_activate();
            break;

        // SYSCALL 9
        case SYSCALL_DEACTIVATE_KEYBOARD: 
        ;
            keyboard_state_deactivate();
            break;

        // SYSCALL 10
        case SYSCALL_GET_KEYBOARD_BORDERS:
        ;
            struct SyscallKeyboardBordersArgs* args_get = (struct SyscallKeyboardBordersArgs*) frame.cpu.general.ebx;

            args_get->up = keyboard_state.up_limit;
            args_get->down = keyboard_state.down_limit;
            args_get->left = keyboard_state.left_limit;
            args_get->right = keyboard_state.right_limit;
            break;
        
        // SYSCALL 11
        case SYSCALL_SET_KEYBOARD_BORDERS: 
        ;
            struct SyscallKeyboardBordersArgs* args = (struct SyscallKeyboardBordersArgs*) frame.cpu.general.ebx;

            keyboard_state.up_limit = args->up;
            keyboard_state.down_limit = args->down;
            keyboard_state.left_limit = args->left;
            keyboard_state.right_limit = args->right;
            break;
        
        // SYSCALL 13
        case SYSCALL_KEYBOARD_PRESS_CTRL: 
        ;
            *((bool*) frame.cpu.general.ebx) = keyboard_state.press_ctrl;
            break;

        // SYSCALL 14
        case SYSCALL_CLEAR_SCREEN: 
        ;
            // Get limits
            uint8_t up = keyboard_state.up_limit;
            uint8_t down = keyboard_state.down_limit;
            uint8_t left = keyboard_state.left_limit;
            uint8_t right = keyboard_state.right_limit;

            // Clear screen
            for (int i = up; i < down + 1; i++) {
                for (int j = left; j < right + 1; j++) {
                    framebuffer_write(i, j, 0x0, 0x7, 0x0);
                }
            }
            break;

        // SYSCALL 15
        case SYSCALL_SET_CURSOR: 
        ;
            framebuffer_set_cursor(
                frame.cpu.general.ebx,
                frame.cpu.general.ecx
            );
            keyboard_state.row = frame.cpu.general.ebx;
            keyboard_state.col = frame.cpu.general.ecx;
            break;

        // SYSCALL 16
        case SYSCALL_GET_CURSOR_ROW: 
        ;
            *((uint8_t*) frame.cpu.general.ebx) = keyboard_state.row;
            break;

        // SYSCALL 17
        case SYSCALL_GET_CURSOR_COL: 
        ;
            *((uint8_t*) frame.cpu.general.ebx) = keyboard_state.col;
            break;

        // SYSCALL 18
        case SYSCALL_READ_CLUSTER: 
        ;
            read_clusters((struct ClusterBuffer*) frame.cpu.general.ebx, frame.cpu.general.ecx, 1);
            break;

        // SYSCALL 19
        case SYSCALL_TERMINATE_PROCESS: 
        ;
            process_destroy(process_manager_state.current_running_pid);
            break;

        // SYSCALL 20
        case SYSCALL_CREATE_PROCESS:
        ;
            *((int32_t*) frame.cpu.general.ecx) = process_create_user_process(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;

        // SYSCALL 21
        case SYSCALL_GET_MAX_PROCESS_COUNT: 
        ;
            *((uint32_t*) frame.cpu.general.ebx) = PROCESS_COUNT_MAX;
            break;

        // SYSCALL 22
        case SYSCALL_GET_PROCESS_INFO: 
        ;
            get_process_info((struct SyscallProcessInfoArgs*) frame.cpu.general.ebx);
            break;

        // SYSCALL 23
        case SYSCALL_GET_CLOCK_TIME:
        ;
            struct SyscallClockTimeArgs* clock_args = (struct SyscallClockTimeArgs*) frame.cpu.general.ebx;

            read_rtc(clock_args);
            break;
        
        // SYSCALL 24
        case SYSCALL_GET_IS_SHELL_OPEN:
        ;
            *((bool*) frame.cpu.general.ebx) = user_variables.shell_open;
            break;

        // SYSCALL 25
        case SYSCALL_SET_IS_SHELL_OPEN:
        ;
            user_variables.shell_open = (bool) frame.cpu.general.ebx;
            break;

        // SYSCALL 26
        case SYSCALL_WRITE_CLUSTER:
        ;
            write_clusters((struct ClusterBuffer*) frame.cpu.general.ebx, frame.cpu.general.ecx, 1);
            break;

        // SYSCALL 27
        case SYSCALL_KILL_PROCESS:
        ;
            process_destroy(frame.cpu.general.ebx);
            break;
    }
}
