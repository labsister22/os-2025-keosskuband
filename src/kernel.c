/*
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
*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/kernel-entrypoint.h"
#include "header/cpu/gdt.h"
#include "header/graphics/graphics.h"  
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"
#include "header/memory/paging.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"
#include "misc/apple.h"

/*
// Helper functions for printing to the framebuffer
*/
void print_string(const char* message, int row, int col, uint8_t color) {
    int i = 0;
    while (message[i] != '\0') {
        framebuffer_write(row, col + i, message[i], color, 0);
        i++;
    }
}

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    graphics_initialize();
    graphics_clear(COLOR_BLACK);
    // framebuffer_clear();
    // framebuffer_set_cursor(0, 0);
    initialize_filesystem_ext2();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    struct EXT2DriverRequest request1 = {
        .buf                   = apple_frames,
        .name                  = "apple.txt",
        .parent_inode          = 1,
        .buffer_size           = 1095*512,
        .name_len              = 9,
        .is_directory          = false,
    };
    write(&request1);


    // Write shell into memory
    struct EXT2DriverRequest request2 = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .parent_inode          = 1,
        .buffer_size           = 0x100000,
        .name_len              = 5,
        .is_directory          = false,
    };

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();

    process_create_user_process(request2);
    scheduler_init();
    scheduler_switch_to_next_process();

    // paging_use_page_directory(_process_list[0].context.page_directory_virtual_addr);
    // kernel_execute_user_program((void*) 0x0);
}