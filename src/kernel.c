#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/graphics/graphics.h"   // Include graphics header
#include "header/kernel-entrypoint.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"

// Remove framebuffer.h include and add font.h

// Track cursor position in graphics coordinates (pixels)
// static uint16_t cursor_x = 0;
// static uint16_t cursor_y = 0;

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);

    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // testing
    graphics_initialize();
    graphics_clear(COLOR_BLACK);
    graphics_char(10, 10, 'A', COLOR_WHITE, COLOR_BLACK); 
    graphics_line(0, 0, 319, 199, COLOR_RED);
    graphics_rect(50, 50, 50, 50, COLOR_GREEN);
    while(1);  
    
    // graphics_clear(COLOR_BLACK);  // Clear screen with black

    // keyboard_state_activate();

    // struct BlockBuffer b;
    // for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    // write_blocks(&b, 17, 1);

    // while (true) {
    //     char c;
    //     get_keyboard_buffer(&c);

    //     // Handle backspace
    //     if (c == '\b') {
    //         if (cursor_x >= 8) {
    //             cursor_x -= 8;  // Move back one character (8 pixels)
    //         } else if (cursor_y >= 8) {
    //             cursor_y -= 8;
    //             cursor_x = VGA_WIDTH - 8;  // Wrap to end of previous line
    //         }

    //         // Erase character by drawing a black rectangle
    //         graphics_rect_fill(cursor_x, cursor_y, 8, 8, COLOR_BLACK);
    //     }
    //     // Handle printable characters
    //     else if (c) {
    //         // Draw character at current cursor position
    //         graphics_char(cursor_x, cursor_y, c, COLOR_WHITE, COLOR_BLACK);

    //         // Advance cursor
    //         cursor_x += 8;
    //         if (cursor_x >= VGA_WIDTH - 8) {  // Wrap to next line
    //             cursor_x = 0;
    //             cursor_y += 8;
    //         }

    //         // Optional: Scroll if reaching the bottom
    //         if (cursor_y >= VGA_HEIGHT - 8) {
    //             graphics_clear(COLOR_BLACK);  // Simple clear (or implement scrolling)
    //             cursor_x = 0;
    //             cursor_y = 0;
    //         }
    //     }
    // }
}