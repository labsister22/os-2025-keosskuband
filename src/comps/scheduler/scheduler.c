#include "header/scheduler/scheduler.h"
#include "header/cpu/portio.h"
#include "header/interrupt/interrupt.h"
#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"

// Static variables for Round Robin scheduling
static uint32_t current_process_idx = 0;

void activate_timer_interrupt(void) {
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) (pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) ((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}

void timer_isr(struct InterruptFrame frame) { 
    // Create context from interrupt frame
    struct Context ctx;
    ctx.cpu = frame.cpu;
    ctx.eip = frame.int_stack.eip;
    ctx.eflags = frame.int_stack.eflags;
    ctx.page_directory_virtual_addr = paging_get_current_page_directory_addr();
    
    // Save context and switch
    scheduler_save_context_to_current_running_pcb(ctx);

    // Acknowledge timer interrupt  
    pic_ack(IRQ_TIMER);

    // switch process
    scheduler_switch_to_next_process();
}

void scheduler_init(void) {
    current_process_idx = 0;
    
    // Find the first ready process and set it to RUNNING
    bool found_process = false;
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state._process_used[i] && 
            _process_list[i].metadata.state == READY) {
            _process_list[i].metadata.state = RUNNING;
            current_process_idx = i;
            found_process = true;
            break;
        }
    }
    
    if (!found_process) {
        // No process to run - system halt
        __asm__ volatile("cli; hlt");
        while(1) {}
    }
    
    // // Set TSS for proper kernel stack switching
    // set_tss_kernel_current_stack();
    
    // Activate timer interrupt
    activate_timer_interrupt();
}

void scheduler_save_context_to_current_running_pcb(struct Context ctx) {
    // Find current running process
    struct ProcessControlBlock *current_pcb = NULL;
    
    for (uint32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (process_manager_state._process_used[i] && 
            _process_list[i].metadata.state == RUNNING) {
            current_pcb = &_process_list[i];
            current_process_idx = i;
            break;
        }
    }
    
    if (current_pcb != NULL) {
        // Save the context
        current_pcb->context = ctx;
        // Set state to READY (will be scheduled again later)
        current_pcb->metadata.state = READY;
    }
}

__attribute__((noreturn)) void scheduler_switch_to_next_process(void) {
    uint32_t next_process_idx = current_process_idx;
    bool found_next = false;
    
    // Round Robin: search for next ready process
    for (uint32_t attempts = 0; attempts < PROCESS_COUNT_MAX; attempts++) {
        next_process_idx = (next_process_idx + 1) % PROCESS_COUNT_MAX;
        
        if (process_manager_state._process_used[next_process_idx] && 
            _process_list[next_process_idx].metadata.state == READY) {
            found_next = true;
            break;
        }
    }
    
    // If no ready process found, check if current process can continue
    if (!found_next) {
        if (process_manager_state._process_used[current_process_idx] &&
            (_process_list[current_process_idx].metadata.state == READY ||
             _process_list[current_process_idx].metadata.state == RUNNING)) {
            next_process_idx = current_process_idx;
            found_next = true;
        }
    }
    
    // If still no process found, halt system
    if (!found_next) {
        __asm__ volatile("cli; hlt");
        while(1) {}
    }
    
    // Update process states
    current_process_idx = next_process_idx;
    _process_list[current_process_idx].metadata.state = RUNNING;
    
    // Switch to the process's page directory
    paging_use_page_directory(_process_list[current_process_idx].context.page_directory_virtual_addr);
    
    // Update TSS for proper stack switching
    // set_tss_kernel_current_stack();
    
    // Context switch to selected process
    process_context_switch(_process_list[current_process_idx].context);
}