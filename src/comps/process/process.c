#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

// Process list and manager state
struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX] = {0};

struct ProcessManagerState process_manager_state = {
    .active_process_count = 0,
    ._process_used = {false}
};

uint32_t process_list_get_inactive_index(void) {
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (!process_manager_state._process_used[i]) {
            return i;
        }
    }
    return -1; // No free slot
}

uint32_t process_generate_new_pid(void) {
    static uint32_t pid_counter = 1;
    return pid_counter++;
}

uint32_t ceil_div(uint32_t a, uint32_t b) {
    if (b == 0) return 0; // Avoid division by zero
    return (a + b - 1) / b;
}

struct ProcessControlBlock* process_get_current_running_pcb_pointer(void) {
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state._process_used[i] &&
            _process_list[i].metadata.state == RUNNING) {
            return &_process_list[i];
        }
    }
    return NULL;
}

int32_t process_create_user_process(struct EXT2DriverRequest request) {
    int32_t retcode = PROCESS_CREATE_SUCCESS;
    
    // 0. VALIDATE AND CHECK FOR ERROR
    // Check if we've reached max process count
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

    // 0.5 GET UNUSED PCB 
    // Get free PCB slot
    int32_t p_index = process_list_get_inactive_index();
    if (p_index == -1) {
        retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
        goto exit_cleanup;
    }
    
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);


    // 1.  ASSIGN VIRTUAL MEMORY 
    // 1.1. CREATE PAGE DIRECTORY
    // Save current page directory
    struct PageDirectory* current_pd = paging_get_current_page_directory_addr();
    
    // Create new page directory for the process
    struct PageDirectory* new_pd = paging_create_new_page_directory();
    if (new_pd == NULL) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    // Store page directory in context
    new_pcb->context.page_directory_virtual_addr = new_pd;

    // 1.2. ASSIGN PAGE FRAMES AS NEEDED
    // Allocate memory for the process
    uint32_t current_address = 0;
    uint32_t remaining_size = request.buffer_size;
    new_pcb->memory.page_frame_used_count = 0;

    // Allocate pages for program code/data
    while (remaining_size > 0 && new_pcb->memory.page_frame_used_count < PROCESS_PAGE_FRAME_COUNT_MAX - 1) {
        void *virtual_addr = (void*)(current_address);
        
        // Allocate a page frame
        if (!paging_allocate_user_page_frame(new_pd, virtual_addr)) {
            // Allocation failed, cleanup
            for (uint32_t i = 0; i < new_pcb->memory.page_frame_used_count; i++) {
                paging_free_user_page_frame(new_pd, new_pcb->memory.virtual_addr_used[i]);
            }
            paging_free_page_directory(new_pd);
            retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
            goto exit_cleanup;
        }

        // Record allocated virtual address
        new_pcb->memory.virtual_addr_used[new_pcb->memory.page_frame_used_count] = virtual_addr;
        new_pcb->memory.page_frame_used_count++;

        current_address += PAGE_FRAME_SIZE;
        remaining_size = (remaining_size > PAGE_FRAME_SIZE) ? (remaining_size - PAGE_FRAME_SIZE) : 0;
    }

    // Allocate stack frame (at 0xBFFFF000 - top of user space, 4MB aligned)
    void *stack_virtual_addr = (void*)0xBFC00000; // 0xC0000000 - 0x400000
    if (!paging_allocate_user_page_frame(new_pd, stack_virtual_addr)) {
        // Cleanup on failure
        for (uint32_t i = 0; i < new_pcb->memory.page_frame_used_count; i++) {
            paging_free_user_page_frame(new_pd, new_pcb->memory.virtual_addr_used[i]);
        }
        paging_free_page_directory(new_pd);
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }
    
    // Record stack allocation
    new_pcb->memory.virtual_addr_used[new_pcb->memory.page_frame_used_count] = stack_virtual_addr;
    new_pcb->memory.page_frame_used_count++;

    // 2. READ & ALLOCATE EXECUTABLE FILE
    // Switch to new page directory to copy data
    paging_use_page_directory(new_pd);
    
    // Read from filesystem and copy to the allocated memory
    struct EXT2DriverRequest read_req = {
        .buf = (void*)0, // Start of user space
        .name = request.name,
        .parent_inode = request.parent_inode,
        .buffer_size = request.buffer_size,
        .name_len = request.name_len,
        .is_directory = false
    };
    
    int8_t read_result = read(read_req);
    
    // Switch back to kernel page directory
    paging_use_page_directory(current_pd);
    
    if (read_result != 0) {
        // Cleanup allocated pages
        for (uint32_t i = 0; i < new_pcb->memory.page_frame_used_count; i++) {
            paging_free_user_page_frame(new_pd, new_pcb->memory.virtual_addr_used[i]);
        }
        paging_free_page_directory(new_pd);
        retcode = PROCESS_CREATE_FAIL_FS_READ_FAILURE;
        goto exit_cleanup;
    }


    // 3. ADD CONTEXT TO PROCESS
    // Initialize process context
    new_pcb->context.cpu = (struct CPURegister) {
        .index = {.edi = 0, .esi = 0},
        .stack = {.ebp = 0xBFFFFFFC, .esp = 0xBFFFFFFC}, // Stack at top of allocated stack frame
        .general = {.ebx = 0, .edx = 0, .ecx = 0, .eax = 0},
        .segment = {
            .gs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,  // User data segment with RPL 3
            .fs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
            .es = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3,
            .ds = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3
        }
    };
    new_pcb->context.eip = (uint32_t) request.buf; // Entry point at start of user space (0x0) 
    new_pcb->context.eflags = CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
    

    // 4. SET METADATA
    // Set metadata
    new_pcb->metadata.pid = process_generate_new_pid();
    memcpy(new_pcb->metadata.name, request.name, request.name_len < 8 ? request.name_len : 8);
    new_pcb->metadata.state = READY;

    // Mark process slot as used
    process_manager_state._process_used[p_index] = true;
    process_manager_state.active_process_count++;

exit_cleanup:
    return retcode;
}

bool process_destroy(uint32_t pid) {
    // Find the process by PID
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state._process_used[i] && 
            _process_list[i].metadata.pid == pid) {
            
            struct ProcessControlBlock *pcb = &_process_list[i];
            
            // Don't destroy the currently running process
            if (pcb->metadata.state == RUNNING) {
                return false;
            }
            
            // Free all allocated page frames
            for (uint32_t j = 0; j < pcb->memory.page_frame_used_count; j++) {
                if (pcb->memory.virtual_addr_used[j] != NULL) {
                    paging_free_user_page_frame(
                        pcb->context.page_directory_virtual_addr,
                        pcb->memory.virtual_addr_used[j]
                    );
                }
            }
            
            // Free the page directory
            if (pcb->context.page_directory_virtual_addr != NULL) {
                paging_free_page_directory(pcb->context.page_directory_virtual_addr);
            }
            
            // Clear the PCB
            memset(pcb, 0, sizeof(struct ProcessControlBlock));
            
            // Update manager state
            process_manager_state._process_used[i] = false;
            process_manager_state.active_process_count--;
            
            return true;
        }
    }
    
    return false; // Process not found
}

// Additional helper functions

struct ProcessControlBlock* process_get_pcb_from_pid(uint32_t pid) {
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state._process_used[i] && 
            _process_list[i].metadata.pid == pid) {
            return &_process_list[i];
        }
    }
    return NULL;
}