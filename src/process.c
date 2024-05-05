#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

struct ProcessManagerState process_manager_state = {
    .process_list_used = {0},
    .active_process_count = 0,
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX] = {0};

int32_t process_list_get_inactive_index(void) {
    for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state.process_list_used[i] == false) {
            process_manager_state.process_list_used[i] = true;
            process_manager_state.active_process_count++;
            return i;
        }
    }
    return -1;
}

int32_t process_create_user_process(struct FAT32DriverRequest request) {
    int32_t retcode = PROCESS_CREATE_SUCCESS; 
    if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
        goto exit_cleanup;
    }

    // Ensure entrypoint is not located at kernel's section at higher half
    if ((uint32_t) request.buf >= KERNEL_VIRTUAL_ADDRESS_BASE) {
        retcode = PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT;
        goto exit_cleanup;
    }

    // Check whether memory is enough for the executable and additional frame for user stack
    uint32_t page_frame_count_needed = ceil_div(request.buffer_size + PAGE_FRAME_SIZE, PAGE_FRAME_SIZE);
    if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    // Process PCB 
    int32_t p_index = process_list_get_inactive_index();
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);

    // 1. Create new page directory
    struct PageDirectory* new_page_dir = paging_create_new_page_directory();

    // 2. Load executable from filesystem
    // Save previous page directory
    struct PageDirectory* prev_page_dir = paging_get_current_page_directory_addr();

    // Load newly created process page directory
    paging_use_page_directory(new_page_dir);

    // Allocate page frame at buf address
    paging_allocate_user_page_frame(new_page_dir, request.buf); 

    // Read file from memory
    if (read(request) != 0) {
        retcode = PROCESS_CREATE_FAIL_FS_READ_FAILURE;
        goto exit_cleanup;
    }

    // Restore previous page directory
    paging_use_page_directory(prev_page_dir);

    // 3. Initialize process context
    // CPU Registers
    new_pcb->context.cpu = (struct CPURegister) {
        .segment = {
            .ds = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
            .es = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
            .fs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
            .gs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
        },
        .stack = {
            .ebp = KERNEL_VIRTUAL_ADDRESS_BASE - 4,
            .esp = KERNEL_VIRTUAL_ADDRESS_BASE - 4,
        },
    };
    // EIP
    new_pcb->context.eip = (uint32_t) request.buf;

    // EFLAGS
    new_pcb->context.eflags = 0 | (CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE);

    // Page Directory
    new_pcb->context.page_directory_virtual_addr = new_page_dir;

    // 4. Initialize Process Metadata
    // Process ID
    // new_pcb->metadata.pid = process_generate_new_pid();
    new_pcb->metadata.pid = p_index;
    
    // Process State   
    new_pcb->metadata.state = PROCESS_STATE_READY;

    // Process Name
    memset(new_pcb->metadata.name, 0, PROCESS_NAME_LENGTH_MAX);
    memcpy(new_pcb->metadata.name, request.name, 8);


exit_cleanup:
    return retcode;
}