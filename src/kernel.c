#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/graphics/graphics.h"  
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

    graphics_initialize();
    graphics_clear(COLOR_BLACK);
    
    graphics_set_cursor(0, 0);
    graphics_store_char_at_cursor(0); 
    graphics_draw_cursor();

    keyboard_state_activate();

    struct BlockBuffer b;
    for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    write_blocks(&b, 17, 1);

    while (true) {
        char c;
        get_keyboard_buffer(&c);

        // Only handle input if there is any
        if (c != 0) {
            graphics_erase_cursor();

            // Handle backspace
            if (c == '\b') {
                uint16_t x, y;
                graphics_get_cursor(&x, &y);
                
                if (x >= 8) {
                    graphics_move_cursor(-8, 0);
                } else if (y >= 8) {
                    graphics_set_cursor(VGA_WIDTH - 8, y - 8);
                }

                graphics_get_cursor(&x, &y);
                
                graphics_rect_fill(x, y, 8, 8, COLOR_BLACK);
                graphics_store_char_at_cursor(0); 
            }
            // Handle printable characters
            else if (c >= ' ' && c <= '~') {
                uint16_t x, y;
                graphics_get_cursor(&x, &y);
                
                graphics_char(x, y, c, COLOR_PINK, COLOR_BLACK);
                
                graphics_move_cursor(8, 0);
                
                graphics_store_char_at_cursor(0);
                
                graphics_get_cursor(&x, &y);
                
                if (x >= VGA_WIDTH - 8) { 
                    graphics_set_cursor(0, y + 8);
                }

                // Check if we need to scroll
                graphics_get_cursor(&x, &y);
                if (y >= VGA_HEIGHT - 8) {
                    graphics_clear(COLOR_BLACK);  
                    graphics_set_cursor(0, 0);
                    graphics_store_char_at_cursor(0); 
                }
            }
        
            graphics_draw_cursor();
        }
        graphics_blink_cursor();
    }
}