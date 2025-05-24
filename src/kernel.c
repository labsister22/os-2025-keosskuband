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

/*

// Function to print a hexadecimal number
void print_hex(uint32_t number, int row, int col, uint8_t color) {
    // Convert number to hex string
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];  // "0x" + 8 hex digits + null terminator
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[10] = '\0';
    
    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[number & 0xF];
        number >>= 4;
    }
    
    // Print the hex string
    print_string(buffer, row, col, color);
}

// Test function for paging system
void test_paging() {
    framebuffer_clear();
    
    // Header
    print_string("===== PAGING TEST =====", 0, 0, 0xF);
    
    // Print basic information
    print_string("Memory System Information:", 2, 0, 0xB);
    print_string("System Memory: 128 MiB", 3, 2, 0xF);
    print_string("Page Size: 4 MiB", 4, 2, 0xF);
    print_string("Page Frames: 32", 5, 2, 0xF);
    print_string("Kernel at: 0xC0100000 (Higher Half)", 6, 2, 0xF);
    
    // Test 1: Physical page allocation
    print_string("Test 1: Page Allocation", 8, 0, 0xB);
    
    // Try to allocate a user page at 0x400000 (4MB boundary - first user page)
    void *user_address = (void*)0x400000;
    bool result = paging_allocate_user_page_frame(&_paging_kernel_page_directory, user_address);
    
    if (result) {
        print_string("Allocated page at 0x400000", 9, 2, 0xA);
        
        // Write test pattern
        uint32_t *test_ptr = (uint32_t*)user_address;
        *test_ptr = 0xABCD1234;
        
        print_string("Wrote test pattern: 0xABCD1234", 10, 2, 0xF);
        print_string("Read back value: ", 11, 2, 0xF);
        print_hex(*test_ptr, 11, 18, 0xE);
        
        // Verify
        if (*test_ptr == 0xABCD1234) {
            print_string("Memory verification: PASSED", 12, 2, 0xA);
        } else {
            print_string("Memory verification: FAILED", 12, 2, 0xC);
        }
        
        // Free the page
        result = paging_free_user_page_frame(&_paging_kernel_page_directory, user_address);
        if (result) {
            print_string("Successfully freed page at 0x400000", 13, 2, 0xA);
        } else {
            print_string("Failed to free page at 0x400000", 13, 2, 0xC);
        }
    } else {
        print_string("Failed to allocate page at 0x400000", 9, 2, 0xC);
    }
    
    // Test 2: Paging protection
    print_string("Test 2: Memory Protection", 15, 0, 0xB);
    
    // Try to allocate a page in kernel space (should fail)
    void *kernel_address = (void*)0xC0400000;
    result = paging_allocate_user_page_frame(&_paging_kernel_page_directory, kernel_address);
    
    if (!result) {
        print_string("Protected kernel space at 0xC0400000: PASSED", 16, 2, 0xA);
    } else {
        print_string("Protected kernel space at 0xC0400000: FAILED", 16, 2, 0xC);
        // Clean up if somehow it worked
        paging_free_user_page_frame(&_paging_kernel_page_directory, kernel_address);
    }
    
    // Test 3: Check misaligned allocation (should fail)
    print_string("Test 3: Misaligned Address", 18, 0, 0xB);
    
    void *misaligned_address = (void*)0x400001; // Not 4MB aligned
    result = paging_allocate_user_page_frame(&_paging_kernel_page_directory, misaligned_address);
    
    if (!result) {
        print_string("Rejected misaligned address 0x400001: PASSED", 19, 2, 0xA);
    } else {
        print_string("Incorrectly allowed misaligned address: FAILED", 19, 2, 0xC);
        // Clean up
        paging_free_user_page_frame(&_paging_kernel_page_directory, misaligned_address);
    }
    
    // Test 4: Multiple page allocations
    print_string("Test 4: Multiple Page Allocations", 21, 0, 0xB);
    
    // Try to allocate several pages
    const int NUM_PAGES = 3;
    void *pages[NUM_PAGES];
    int success_count = 0;
    
    for (int i = 0; i < NUM_PAGES; i++) {
        pages[i] = (void*)(0x800000 + (i * PAGE_FRAME_SIZE)); // Starting from 8MB
        if (paging_allocate_user_page_frame(&_paging_kernel_page_directory, pages[i])) {
            success_count++;
            
            // Write a unique value to each page
            *((uint32_t*)pages[i]) = 0xBEEF0000 + i;
        }
    }
    
    print_string("Allocated pages: ", 22, 2, 0xF);
    if (success_count == 0) {
        print_string("None", 22, 18, 0xC);
    } else {
        char count[2] = {'0' + success_count, '\0'};
        print_string(count, 22, 18, 0xA);
        print_string("out of 3", 22, 20, 0xF);
        
        // Verify all allocated pages
        bool all_valid = true;
        for (int i = 0; i < success_count; i++) {
            uint32_t expected = 0xBEEF0000 + i;
            uint32_t actual = *((uint32_t*)pages[i]);
            if (actual != expected) {
                all_valid = false;
                break;
            }
        }
        
        if (all_valid) {
            print_string("All page values verified correctly", 23, 2, 0xA);
        } else {
            print_string("Page value verification failed", 23, 2, 0xC);
        }
        
        // Free all allocated pages
        for (int i = 0; i < success_count; i++) {
            paging_free_user_page_frame(&_paging_kernel_page_directory, pages[i]);
        }
        print_string("All pages freed", 24, 2, 0xA);
    }
    
    // Test completion message
    print_string("===== PAGING TEST COMPLETE =====", 26, 0, 0xF);
}

void kernel_setup(void) {
    // First initialize paging - this is required before using the framebuffer
    // The basic paging is already set up in kernel-entrypoint.s, but we need to
    // complete the initialization
    paging_initialize();
    
    // Now we can load the GDT since paging is set up
    load_gdt(&_gdt_gdtr);

    // Set up interrupts
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    
    // Initialize keyboard
    keyboard_state_activate();

    // Initialize filesystem
    initialize_filesystem_ext2();
    
    // Framebuffer now uses the higher half address 0xC00B8000
    framebuffer_clear();
    
    // Print welcome message
    print_string("Welcome to the OS with Paging Enabled!", 0, 0, 0xF);
    print_string("Higher Half Kernel is running at 0xC0100000", 1, 0, 0xA);
    print_string("Press any key to run paging tests...", 3, 0, 0xE);
    
    // Wait for a keypress before running tests
    char c = 0;
    while (c == 0) {
        get_keyboard_buffer(&c);
    }
    
    // Run the paging tests
    test_paging();
    
    print_string("Paging tests completed. Press keys to see them echo here:", 0, 0, 0xF);
    
    // Main loop - echo key presses
    int row = 2, col = 0;
    while (true) {
        char c;
        get_keyboard_buffer(&c);

        // Handle keyboard input
        if (c != 0) {
            framebuffer_write(row, col++, c, 0xE, 0);
            if (col >= FRAMEBUFFER_WIDTH) {
                col = 0;
                row++;
                if (row >= FRAMEBUFFER_HEIGHT) {
                    framebuffer_clear();
                    print_string("Paging tests completed. Press keys to see them echo here:", 0, 0, 0xF);
                    row = 2;
                    col = 0;
                }
            }
        }
    }
}
*/

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
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
        .parent_inode                 = 1,
        .buffer_size           = 0x100000,
        .name_len              = 5,
        .is_directory          = false,
    };
    read(request2);

    struct EXT2DriverRequest request_test = {
        .buf                   = "something1",
        .name                  = "check1",
        .parent_inode          = 1,
        .buffer_size           = 11,
        .name_len              = 7,
        .is_directory          = false,
    };
    write(&request_test);

    request_test.buf = "something2";
    request_test.name = "check2";
    write(&request_test);

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t*) 0);
}