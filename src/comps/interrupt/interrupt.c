#include "header/interrupt/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"

typedef struct {
    int32_t row;
    int32_t col;
} CP;

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        case 0:
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct EXT2DriverRequest*) frame.cpu.general.ebx
            );
            break;          
        case 4:
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case 5:
            // 1 character writing
            put_char(
                (char*) frame.cpu.general.ebx, 
                frame.cpu.general.ecx, 
                ((CP*) frame.cpu.general.edx)->row, 
                ((CP*) frame.cpu.general.edx)->col
            );
            break;
        case 6:
            // please use write_buffer for this
            puts(
                (char*) frame.cpu.general.ebx, 
                frame.cpu.general.ecx, 
                ((CP*) frame.cpu.general.edx)->row, 
                ((CP*) frame.cpu.general.edx)->col
            );
            break;
        case 7: 
            keyboard_state_activate();
            break;
        case 8: // NEW: Set cursor syscall untuk shell
            framebuffer_set_cursor(
                (uint8_t)frame.cpu.general.ebx,  // row
                (uint8_t)frame.cpu.general.ecx   // col
            );
            break;
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