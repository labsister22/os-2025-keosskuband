#include "header/interrupt/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"
#include "header/graphics/graphics.h"

typedef struct {
    int32_t row;
    int32_t col;
} CP;

typedef struct {
    int size;
    uint8_t font_color;
    uint8_t bg_color;
} PrintRequest;

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        case 0:
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 1:
            *((int8_t*) frame.cpu.general.ecx) = read_directory(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break; 
        case 2:  
            *((int8_t*) frame.cpu.general.ecx) = write(
                (struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 3:
            *((int8_t*) frame.cpu.general.ecx) = delete(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 4:
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case 5:
            // 1 character writing
            char ch = *((char*) frame.cpu.general.ebx);
            uint8_t color = frame.cpu.general.ecx;
            CP* cursor_pos = (CP*) frame.cpu.general.edx;
            
            graphics_char(
                cursor_pos->col * 5,  // Convert column to X pixel
                cursor_pos->row * 8,  // Convert row to Y pixel
                ch,
                color,
                COLOR_BLACK
            );
            break;
        case 6:
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
        case 7: 
            keyboard_state_activate();
            break;
        case 8:
            graphics_set_cursor(
                (uint16_t)frame.cpu.general.ebx, 
                (uint16_t)frame.cpu.general.ecx
            );
            break;
        case 9:
            graphics_char(
                ((CP*) frame.cpu.general.edx)->col * 5, 
                ((CP*) frame.cpu.general.edx)->row * 8, 
                *((char*) frame.cpu.general.ebx), 
                ((PrintRequest*) frame.cpu.general.ecx)->font_color, 
                ((PrintRequest*) frame.cpu.general.ecx)->bg_color
            );
            break;
        case 10: 
            graphics_draw_cursor();
            break;
        case 11: 
            graphics_erase_cursor();
            break;
        case 12: 
            graphics_store_char_at_cursor((char)frame.cpu.general.ebx);
            graphics_set_cursor_colors(
                (uint8_t)frame.cpu.general.ecx,
                COLOR_BLACK                       
            );
            break;
        case 13: 
            graphics_set_cursor(
                (uint16_t)frame.cpu.general.ebx,  
                (uint16_t)frame.cpu.general.ecx  
            );
            break;
        case 14: 
            graphics_blink_cursor();
            break;
        case 15: 
            graphics_set_cursor_colors(
                (uint8_t)frame.cpu.general.ebx,  
                (uint8_t)frame.cpu.general.ecx   
            );
            break;
        case 16:
            graphics_pixel(
                (uint16_t)frame.cpu.general.ebx, 
                (uint16_t)frame.cpu.general.ecx, 
                (uint8_t)frame.cpu.general.edx
            );
        break;
        case 17:
            graphics_clear(
                (uint8_t)frame.cpu.general.ebx
            );
            break;
        case 18:
            graphics_scroll(
                (uint8_t)frame.cpu.general.ebx, 
                (uint8_t)frame.cpu.general.ecx
            );
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

void main_interrupt_handler(struct InterruptFrame frame) {
    switch (frame.int_number) {
        // Timer interrupt
        case PIC1_OFFSET + IRQ_TIMER:
            pic_ack(IRQ_TIMER);
            break;
            
        // Keyboard interrupt
        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;

        // Other interrupts
        case 0x30: // syscall
            syscall(frame);
            break;
            
        default:
            if (frame.int_number >= PIC1_OFFSET) {
                pic_ack(frame.int_number - PIC1_OFFSET);
            }
            break;
    }
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}