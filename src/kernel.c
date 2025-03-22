#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/text/framebuffer.h"
#include "header/kernel-entrypoint.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"


void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);

    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    
    
    int row = 0, col = 0;
    keyboard_state_activate();

    struct BlockBuffer b;
    for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    write_blocks(&b, 17, 1);

    while (true) {
        char c;
        get_keyboard_buffer(&c);

        // backspace  -jibs
        if (c == '\b') {
            if (col == 0 && row == 0) { // not sure what to do actually
                row = 0;
                col = 0;
            }
            else if (col == 0) { // go back one row
                --row;
                col = FRAMEBUFFER_WIDTH;
            } 
            else { // go back one col
                --col;
            }
            framebuffer_write(row, col, '\0', 0xF, 0);  // replace printed char with \0 which is just empty
            framebuffer_set_cursor(row, col);
        }
        // other char
        else if (c) {
            framebuffer_write(row, col, c, 0xF, 0);
            if (col >= FRAMEBUFFER_WIDTH) {
                ++row;
                col = 0;
            } else {
                ++col;
            }
           framebuffer_set_cursor(row, col);
        }
    }
}