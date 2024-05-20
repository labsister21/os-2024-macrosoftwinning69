#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

struct ProcessManagerState process_manager_state = {
    .process_list_used = {0},
    .active_process_count = 0,
    .current_running_pid = NO_PROCESS_RUNNING
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX] = {0};

int32_t process_list_get_inactive_index(void) {
    for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state.process_list_used[i] == false) {
            return i;
        }
    }
    return -1;
}

struct ProcessControlBlock* process_get_current_running_pcb_pointer(void) {
    // Return NULL if no process is running
    if (process_manager_state.current_running_pid == NO_PROCESS_RUNNING) {
        return NULL;
    }
    
    return &(_process_list[process_manager_state.current_running_pid]);
}

bool process_destroy(uint32_t pid) {
    if (pid >= PROCESS_COUNT_MAX) return false;

    // Get process page directory
    struct PageDirectory* page_dir = _process_list[pid].context.page_directory_virtual_addr;
    
    // Free page frames
    for (uint32_t i = 0; i < PROCESS_PAGE_FRAME_COUNT_MAX; i++) {
        if (_process_list[pid].memory.data_virtual_addr_used[i] == true) {
            paging_free_user_page_frame(page_dir, _process_list[pid].memory.virtual_addr_used[i]);
        }
    }

    // Free page directory
    paging_free_page_directory(page_dir);

    // Delete data and decrement count from process manager
    memset(&_process_list[pid], 0, sizeof(struct ProcessControlBlock));
    process_manager_state.process_list_used[pid] = false;
    process_manager_state.active_process_count--;

    return true;
}

void process_allocate_page_frame(struct ProcessControlBlock* pcb, void* virtual_addr) {
    // Auto return if page frame used count already exceed the limit
    if (pcb->memory.page_frame_used_count >= PROCESS_PAGE_FRAME_COUNT_MAX) return;

    for (uint32_t i = 0; i < PROCESS_PAGE_FRAME_COUNT_MAX; i++) {
        if (pcb->memory.data_virtual_addr_used[i] == false) {
            // Update PCB memory metadata
            pcb->memory.data_virtual_addr_used[i] = true;
            pcb->memory.virtual_addr_used[i] = virtual_addr;
            pcb->memory.page_frame_used_count++;

            // Allocate page frame
            paging_allocate_user_page_frame(pcb->context.page_directory_virtual_addr, virtual_addr);
            return;
        }
    }
}

void process_deallocate_page_frame(struct ProcessControlBlock* pcb, void* virtual_addr) {
    for (uint32_t i = 0; i < PROCESS_PAGE_FRAME_COUNT_MAX; i++) {
        if (pcb->memory.data_virtual_addr_used[i] == true && pcb->memory.virtual_addr_used[i] == virtual_addr) {
            // Update PCB memory metadata
            pcb->memory.data_virtual_addr_used[i] = false;
            pcb->memory.virtual_addr_used[i] = NULL;
            pcb->memory.page_frame_used_count--;

            // Deallocate page frame
            paging_free_user_page_frame(pcb->context.page_directory_virtual_addr, virtual_addr);
            return;
        }
    }
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

    process_manager_state.active_process_count++;
    process_manager_state.process_list_used[p_index] = true;

    for (uint32_t i = 0; i < PROCESS_PAGE_FRAME_COUNT_MAX; i++) {
        new_pcb->memory.data_virtual_addr_used[i] = false;
        new_pcb->memory.virtual_addr_used[i] = NULL;
    }
    new_pcb->memory.page_frame_used_count = 0;

    // 1. Create new page directory
    struct PageDirectory* new_page_dir = paging_create_new_page_directory();

    // 2. Load executable from filesystem
    // Save previous page directory
    struct PageDirectory* prev_page_dir = paging_get_current_page_directory_addr();

    // Load newly created process page directory
    paging_use_page_directory(new_page_dir);

    // Set PCB page directory
    new_pcb->context.page_directory_virtual_addr = new_page_dir;

    // Allocate page frame at buf address
    process_allocate_page_frame(new_pcb, request.buf);
    process_allocate_page_frame(new_pcb, (void*) (KERNEL_VIRTUAL_ADDRESS_BASE - 4));

    // Read file from memory
    if (read(request) != 0) {
        paging_use_page_directory(prev_page_dir);
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