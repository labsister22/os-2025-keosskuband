#include "header/interrupt/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"
#include "header/graphics/graphics.h"
#include "header/scheduler/scheduler.h"
#include "header/memory/paging.h"
#include "header/driver/cmos.h"
#include "header/memory/memory-manager.h"
#include "header/process/process-commands/ps.h"
#include "header/process/process-commands/exec.h"
#include "header/process/process-commands/sleep.h"

typedef struct {
    int32_t row;
    int32_t col;
} CP;

typedef struct {
    int size;
    uint8_t font_color;
    uint8_t bg_color;
} PrintRequest;

// Add this to the syscall function in interrupt.c after the existing cases

void syscall(struct InterruptFrame frame) {
        switch (frame.cpu.general.eax) {
        case SYSCALL_READ:
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case SYSCALL_READ_DIR:
            *((int8_t*) frame.cpu.general.ecx) = read_directory(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break; 
        case SYSCALL_WRITE:  
            *((int8_t*) frame.cpu.general.ecx) = write(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case SYSCALL_DELETE:
            *((int8_t*) frame.cpu.general.ecx) = delete(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case SYSCALL_KEYBOARD:
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case SYSCALL_GRAPHICS_CHAR:
            // 1 character writing
            char ch = *((char*) frame.cpu.general.ebx);
            uint8_t color = frame.cpu.general.ecx;
            CP* cursor_pos = (CP*) frame.cpu.general.edx;
            
            graphics_char(
                cursor_pos->col * 5,  // Convert column to X pixel
                cursor_pos->row * 8,  // Convert row to Y pixel
                ch,
                color,
                255
            );
            break;
        case SYSCALL_GRAPHICS_STR:
            char* str = (char*) frame.cpu.general.ebx;
            int size = frame.cpu.general.ecx;
            int row = ((CP*) frame.cpu.general.edx)->row;
            int col = ((CP*) frame.cpu.general.edx)->col;
            
            for (int i = 0; i < size; i++) {
                graphics_char(
                    col * 5, 
                    row * 8, 
                    str[i], 
                    COLOR_WHITE, 
                    COLOR_BLACK
                );
                col++;
                if (col >= 80) {
                    col = 0;
                    row++;
                }
            }

            break;
        case SYSCALL_ACT_KEYBOARD: 
            keyboard_state_activate();
            break;
        case SYSCALL_SET_CURSOR:
            graphics_set_cursor(
                (uint16_t) frame.cpu.general.ebx, 
                (uint16_t) frame.cpu.general.ecx
            );
            break;
        case SYSCALL_PRINT_COLOR:
            graphics_char(
                ((CP*) frame.cpu.general.edx)->col * 5, 
                ((CP*) frame.cpu.general.edx)->row * 8, 
                *((char*) frame.cpu.general.ebx), 
                ((PrintRequest*) frame.cpu.general.ecx)->font_color, 
                ((PrintRequest*) frame.cpu.general.ecx)->bg_color
            );
            break;
        case SYSCALL_DRAW_CURSOR: 
            graphics_draw_cursor();
            break;
        case SYSCALL_ERASE_CURSOR: 
            graphics_erase_cursor();
            break;
        case SYSCALL_STORE_CHAR: 
            graphics_store_char_at_cursor((char)frame.cpu.general.ebx);
            graphics_set_cursor_colors(
                (uint8_t)frame.cpu.general.ecx,
                COLOR_BLACK                       
            );
            break;
        case SYSCALL_CURSOR_BLINK: 
            graphics_blink_cursor();
            break;
        case SYSCALL_CURSOR_COLOR: 
            graphics_set_cursor_colors(
                (uint8_t)frame.cpu.general.ebx,  
                (uint8_t)frame.cpu.general.ecx   
            );
            break;
        case SYSCALL_GRAPHICS_PIXEL:
            graphics_pixel(
                (uint16_t)frame.cpu.general.ebx, 
                (uint16_t)frame.cpu.general.ecx, 
                (uint8_t)frame.cpu.general.edx
            );
        break;
        case SYSCALL_GRAPHICS_CLEAR:
            graphics_clear(
                (uint8_t)frame.cpu.general.ebx
            );
            break;
        case SYSCALL_GRAPHICS_SCROLL:
            graphics_scroll(
                (uint8_t)frame.cpu.general.ebx, 
                (uint8_t)frame.cpu.general.ecx
            );
            break;
        case SYSCALL_FILL_SCREEN:
            graphics_fill_screen_with_color();
            break;
        case SYSCALL_RENDER_FRAME:
            char* frame_buffer = (char*) frame.cpu.general.ebx;
            int buffer_width = (int) frame.cpu.general.edx;
            int offset = 0;
            int index_data;
            for (int i = 0; i < 200; i++) {
                index_data = i * buffer_width;
                for (int j = 0; j < 320; j++) {
                    VGA_MEMORY[offset] = frame_buffer[index_data];
                    offset++;
                    index_data++;
                }
            }
            break;
        case SYSCALL_GET_BUFFER_SIZE:
             *((int32_t*) frame.cpu.general.ecx) = get_buffer_size(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case SYSCALL_PS_CMD:
            ps((ProcessMetadata*) frame.cpu.general.ebx, (uint8_t) frame.cpu.general.ecx);
            break;
        case SYSCALL_GET_MAX_PS:
            uint32_t* max_amount = (uint32_t*) frame.cpu.general.ebx;
            *max_amount = PROCESS_COUNT_MAX;
            break;
        case SYSCALL_TERMINATE_PROCESS:
            struct ProcessControlBlock* cur_pcb = process_get_current_running_pcb_pointer();
            process_destroy(cur_pcb->metadata.pid, 1);
            break;
        case SYSCALL_KILL_PS:
            bool* success = (bool*) frame.cpu.general.ecx;
            *success = process_destroy((uint32_t) frame.cpu.general.ebx, 0);
            break;
        case SYSCALL_EXEC_PS:
            exec(
                (char *) frame.cpu.general.ebx, 
                (uint32_t) frame.cpu.general.ecx, 
                (int32_t*) frame.cpu.general.edx
            );
            break;
        case SYSCALL_GET_TIME: // CMOS Read Time syscall
            {
                // Define TimeInfo structure matching clock.c
                struct {
                    uint8_t second;
                    uint8_t minute;
                    uint8_t hour;
                    uint8_t day;
                    uint8_t month;
                    uint16_t year;
                } *time_info = (void*) frame.cpu.general.ebx;
                
                // Read CMOS data
                struct CMOS cmos_data;
                read_rtc(&cmos_data);
                
                // Copy to user structure
                time_info->second = cmos_data.second;
                time_info->minute = cmos_data.minute;
                time_info->hour = cmos_data.hour;
                time_info->day = cmos_data.day;
                time_info->month = cmos_data.month;
                time_info->year = cmos_data.year;
            }
            break;
        case SYSCALL_SLEEP:
            {
                int* sleep_time = (int*) frame.cpu.general.ebx;

                // Create context from interrupt frame
                struct Context ctx;
                ctx.cpu = frame.cpu;
                ctx.eip = frame.int_stack.eip;
                ctx.eflags = frame.int_stack.eflags;
                ctx.page_directory_virtual_addr = paging_get_current_page_directory_addr();
                
                // Save context
                scheduler_save_context_to_current_running_pcb(ctx);
                
                //sleep
                sleep(*sleep_time);

                // switch
                scheduler_switch_to_next_process();
            }
            break;
        case SYSCALL_GRAPHICS_LINE:
            {
                // Draw line: ebx = x1, ecx = y1, edx = packed x2/y2, esi = color
                uint16_t x1 = (uint16_t)frame.cpu.general.ebx;
                uint16_t y1 = (uint16_t)frame.cpu.general.ecx;
                uint32_t packed_coords = frame.cpu.general.edx;
                uint16_t x2 = (packed_coords >> 16) & 0xFFFF;
                uint16_t y2 = packed_coords & 0xFFFF;
                uint8_t color = (uint8_t)frame.cpu.index.esi;
                
                graphics_line(x1, y1, x2, y2, color);
            }
            break;
        case SYSCALL_GRAPHICS_CIRCLE:
        {
            uint16_t center_x = (uint16_t)frame.cpu.general.ebx;
            uint16_t center_y = (uint16_t)frame.cpu.general.ecx;
            uint16_t radius = (uint16_t)frame.cpu.general.edx;
            uint8_t color = (uint8_t)frame.cpu.index.esi;
            
            graphics_circle_filled(center_x, center_y, radius, color);
        }
        break;
        case SYSCALL_GET_TICK:
            {
                lluint* tick = (lluint*) frame.cpu.general.ebx;
                *tick = time_since_running;
            }
            break;
        case SYSCALL_HEAP_EXPAND:
        {
            void* addr = (void*) frame.cpu.general.ebx;
            size_t size = (size_t) frame.cpu.general.ecx;
            int* result_ptr = (int*) frame.cpu.general.edx;
            
            int result = syscall_heap_expand(addr, size);
            
            if (result_ptr != NULL) {
                *result_ptr = result;
            }
        }
            break;
        case SYSCALL_MALLOC:
        {
            size_t size = (size_t) frame.cpu.general.ebx;
            void** ptr_ptr = (void**) frame.cpu.general.ecx;

            void* ptr = malloc(size);
            if (ptr_ptr != NULL) {
                *ptr_ptr = ptr;
            }
        }
            break;
        case SYSCALL_FREE:
        {
            void* ptr = (void*) frame.cpu.general.ebx;
            free(ptr);
        }
            break;
        case 53:
        //get inode name
        {
            *(uint32_t*) frame.cpu.general.ecx = get_inode_from_name((struct EXT2DriverRequest*) frame.cpu.general.ebx);
        }

    }
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

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

// static volatile bool in_interrupt_handler = false;

void main_interrupt_handler(struct InterruptFrame frame) {
    // // Prevent interrupt nesting issues
    // if (in_interrupt_handler && frame.int_number != (IRQ_KEYBOARD + PIC1_OFFSET)) {
    //     // If we're already in an interrupt handler, only allow keyboard interrupts
    //     pic_ack(frame.int_number - PIC1_OFFSET);
    //     return;
    // }
    
    // in_interrupt_handler = true;
    
    switch (frame.int_number) {
        case PIC1_OFFSET + IRQ_TIMER:  // 32 - Timer
            timer_isr(frame);
            break;
            
        case PIC1_OFFSET + IRQ_KEYBOARD:  // 33 - Keyboard  
            keyboard_isr();
            break;
            
        case 0x30:  // 48 - Syscall
            syscall(frame);
            break;
            
        default:
            // Handle unknown interrupts
            if (frame.int_number >= PIC1_OFFSET && frame.int_number < PIC1_OFFSET + 8) {
                pic_ack(frame.int_number - PIC1_OFFSET);
            } else if (frame.int_number >= PIC2_OFFSET && frame.int_number < PIC2_OFFSET + 8) {
                pic_ack(frame.int_number - PIC2_OFFSET);
            }
            break;
    }
    
    // in_interrupt_handler = false;
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}