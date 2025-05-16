#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/graphics/graphics.h"  
#include "header/kernel-entrypoint.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);

    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // graphics_initialize();
    // graphics_clear(COLOR_BLACK);
    
    // graphics_set_cursor(0, 0);
    // graphics_store_char_at_cursor(0); 
    // graphics_draw_cursor();

    keyboard_state_activate();

    // int row = 10, col = 0;
    keyboard_state_activate();

    initialize_filesystem_ext2();

    struct EXT2DriverRequest req = {
        .parent_inode = 1, // root directory
        .name = "notes.txt",
        .name_len = 9,
        .buf = "mamah, aku gagal </3",
        .buffer_size = 20,
        .is_directory = false
    };
    // write(&req);

    req.parent_inode = 1;
    req.name =  "nigga.txt";
    req.name_len = 9;
    req.buf =   "bismillah";
    req.buffer_size = 9;
    req.is_directory = false;
    // write(&req);

    req.parent_inode = 1;
    req.name =  "folder";
    req.name_len = 6;
    req.buf =   "";
    req.buffer_size = 0;
    req.is_directory = true;
    // write(&req);

    req.parent_inode = 4;
    req.name =  "nigga2.txt";
    req.name_len = 10;
    req.buf =   "bismillah";
    req.buffer_size = 9;
    req.is_directory = false;
    write(&req);

    char buffer[100];
    memset(buffer, 0, sizeof(buffer));

    req = (struct EXT2DriverRequest){
        .parent_inode = 1,
        .name = "nigga2.txt",
        .name_len = 10,
        .buf = buffer,
        .buffer_size = 80
    };
    read(req);

    req = (struct EXT2DriverRequest){
        .parent_inode = 4,
        .name = "nigga2.txt",
        .name_len = 10,
        .buf = buffer,
        .buffer_size = 80
    };
    read(req);

    int x = 10, y = 0;
    write_buffer(buffer, 80, x, y);

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