#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "header/process/process.h"

#define PIT_MAX_FREQUENCY   1193182
#define PIT_TIMER_FREQUENCY 100
#define PIT_TIMER_COUNTER   (PIT_MAX_FREQUENCY / PIT_TIMER_FREQUENCY)

#define PIT_COMMAND_REGISTER_PIO          0x43
#define PIT_COMMAND_VALUE_BINARY_MODE     0b0
#define PIT_COMMAND_VALUE_OPR_SQUARE_WAVE (0b011 << 1)
#define PIT_COMMAND_VALUE_ACC_LOHIBYTE    (0b11  << 4)
#define PIT_COMMAND_VALUE_CHANNEL         (0b00  << 6) 
#define PIT_COMMAND_VALUE (PIT_COMMAND_VALUE_BINARY_MODE | PIT_COMMAND_VALUE_OPR_SQUARE_WAVE | PIT_COMMAND_VALUE_ACC_LOHIBYTE | PIT_COMMAND_VALUE_CHANNEL)

#define PIT_CHANNEL_0_DATA_PIO 0x40

/**
 * Read all general purpose register values and set control register.
 * Resume the execution flow back to ctx.eip and ctx.eflags
 * 
 * @note          Implemented in assembly
 * @param context Target context to switch into
 */
__attribute__((noreturn)) extern void process_context_switch(struct Context ctx);

/* --- Timer --- */

/**
 * Timer interrupt service routine
 * Called when IRQ0 (timer) interrupt occurs
 * 
 * @param frame Interrupt frame containing CPU state
 */
void timer_isr(struct InterruptFrame frame);

/**
 * Activate timer interrupt (IRQ0) using PIT (Programmable Interval Timer)
 */
void activate_timer_interrupt(void);

/* --- Scheduler --- */

/**
 * Initialize scheduler before executing init process 
 */
void scheduler_init(void); 

/**
 * Save context to current running process
 * 
 * @param ctx Context to save to current running process control block
 */
void scheduler_save_context_to_current_running_pcb(struct Context ctx);

/**
 * Trigger the scheduler algorithm and context switch to new process
 * Uses Round Robin scheduling algorithm
 */
__attribute__((noreturn)) void scheduler_switch_to_next_process(void);

#endif