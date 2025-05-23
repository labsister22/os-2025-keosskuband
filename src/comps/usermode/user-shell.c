#include <stdint.h>
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"
#include "comps/usermode/commands/apple.h"
#include "header/usermode/commands/help.h"
#include "header/usermode/commands/clear.h"
#include "header/usermode/commands/echo.h"

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGS_AMOUNT 10
#define MAX_ARGS_LENGTH 32
#define SHELL_PROMPT "Keossku-Band$/ "

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// Add cursor visibility tracking
static bool cursor_shown = false;
static char char_under_cursor = ' '; // Store character that cursor is over
static uint8_t char_color_under_cursor = COLOR_WHITE; // Store original color

CP cursor = {0, 0};
char input_buffer[MAX_INPUT_LENGTH];
int input_length = 0;
char args[MAX_ARGS_AMOUNT][MAX_ARGS_LENGTH];
int cursor_position = 0;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

// New syscall for cursor operations
void graphics_draw_cursor_syscall() {
    syscall(10, 0, 0, 0); // Add syscall 10 for drawing cursor
}

void graphics_erase_cursor_syscall() {
    syscall(11, 0, 0, 0); // Add syscall 11 for erasing cursor
}

void graphics_store_char_syscall(char c, uint8_t color) {
    syscall(12, (uint32_t)c, (uint32_t)color, 0); // Add syscall 12 for storing char and color at cursor
}

void graphics_set_cursor_colors_syscall(uint8_t fg_color, uint8_t bg_color) {
    syscall(15, (uint32_t)fg_color, (uint32_t)bg_color, 0); // Add syscall 15 for setting cursor colors
}

void show_cursor() {
    if (!cursor_shown) {
        // Store the character that will be under the cursor with its color
        if (cursor_position < input_length) {
            char_under_cursor = input_buffer[cursor_position];
            char_color_under_cursor = COLOR_WHITE; // Input text is always white
        } else {
            char_under_cursor = ' ';
            char_color_under_cursor = COLOR_BLACK; // Background
        }
        
        // Set cursor colors to be more visible
        graphics_set_cursor_colors_syscall(COLOR_WHITE, COLOR_GREEN);
        graphics_store_char_syscall(char_under_cursor, char_color_under_cursor);
        graphics_draw_cursor_syscall();
        cursor_shown = true;
    }
}

void hide_cursor() {
    if (cursor_shown) {
        graphics_erase_cursor_syscall();
        cursor_shown = false;
    }
}

void clear_input_buffer() {
    for (int i = 0; i < MAX_INPUT_LENGTH; i++) {
        input_buffer[i] = '\0';
    }
    input_length = 0;
    cursor_position = 0;
}

void set_hardware_cursor() {
    // Update both hardware cursor and graphics cursor
    syscall(8, cursor.row, cursor.col, 0);
    
    // Convert text coordinates to pixel coordinates for graphics cursor
    uint16_t pixel_x = cursor.col * 8;
    uint16_t pixel_y = cursor.row * 8;
    syscall(13, pixel_x, pixel_y, 0); // Add syscall 13 for graphics cursor position
}

// Scroll the screen up by one line
void scroll_screen() {
    cursor.row = 0;
    cursor.col = 0;
}

// Handle cursor position with proper wrapping and scrolling
void advance_cursor() {
    cursor.col++;
    
    // Handle column overflow - wrap to next line
    if (cursor.col >= SCREEN_WIDTH) {
        cursor.col = 0;
        cursor.row++;
        
        // Handle row overflow - scroll screen
        if (cursor.row >= SCREEN_HEIGHT) {
            cursor.row = SCREEN_HEIGHT - 1;
            scroll_screen();
        }
    }
    
    set_hardware_cursor();
}

void print_string_colored(const char* str, uint8_t color) {
    hide_cursor(); // Hide cursor while printing
    
    int i = 0;
    while (str[i] != '\0') {
        syscall(5, (uint32_t)&str[i], color, (uint32_t)&cursor);
        advance_cursor();
        i++;
    }
}

void print_string_at_cursor(const char* str) {
    print_string_colored(str, COLOR_WHITE);
}

void print_newline() {
    hide_cursor(); // Hide cursor while moving
    
    cursor.col = 0;
    cursor.row++;
    
    // Handle screen overflow
    if (cursor.row >= SCREEN_HEIGHT) {
        cursor.row = SCREEN_HEIGHT - 1;
        scroll_screen();
    }
    
    set_hardware_cursor();
}

void print_prompt() {
    print_string_colored(SHELL_PROMPT, COLOR_LIGHT_CYAN);
}

void redraw_input_line(int prompt_start_col) {
    hide_cursor(); // Hide cursor during redraw
    
    int current_row = cursor.row;
    cursor.col = prompt_start_col;

    // Clear from prompt position to end of line
    for (int i = prompt_start_col; i < SCREEN_WIDTH; i++) {
        char space = ' ';
        syscall(5, (uint32_t)&space, COLOR_BLACK, (uint32_t)&cursor); // Use black background
        cursor.col++;
    }
    
    // Reset to prompt position
    cursor.col = prompt_start_col;
    cursor.row = current_row;
    
    // Redraw the input, handling potential line wrapping
    int display_cursor_col = prompt_start_col;
    int display_cursor_row = current_row;
    
    for (int i = 0; i < input_length; i++) {
        syscall(5, (uint32_t)&input_buffer[i], COLOR_WHITE, (uint32_t)&cursor);
        display_cursor_col++;
        
        // Handle wrapping while displaying input
        if (display_cursor_col >= SCREEN_WIDTH) {
            display_cursor_col = 0;
            display_cursor_row++;
            if (display_cursor_row >= SCREEN_HEIGHT) {
                display_cursor_row = SCREEN_HEIGHT - 1;
            }
        }
        
        cursor.col = display_cursor_col;
        cursor.row = display_cursor_row;
    }

    // Position cursor at the correct location based on cursor_position
    display_cursor_col = prompt_start_col + cursor_position;
    display_cursor_row = current_row;
    
    // Handle wrapping for cursor position
    while (display_cursor_col >= SCREEN_WIDTH) {
        display_cursor_col -= SCREEN_WIDTH;
        display_cursor_row++;
        if (display_cursor_row >= SCREEN_HEIGHT) {
            display_cursor_row = SCREEN_HEIGHT - 1;
        }
    }
    
    cursor.col = display_cursor_col;
    cursor.row = display_cursor_row;
    set_hardware_cursor();
    
    show_cursor(); // Show cursor after redraw
}

void insert_char_at_cursor(char c) {
    if (input_length >= MAX_INPUT_LENGTH - 1) {
        return;
    }
    
    hide_cursor(); // Hide cursor during modification
    
    for (int i = input_length; i > cursor_position; i--) {
        input_buffer[i] = input_buffer[i - 1];
    }

    input_buffer[cursor_position] = c;
    input_length++;
    cursor_position++;
    input_buffer[input_length] = '\0';
}

void delete_char_before_cursor() {
    if (cursor_position <= 0 || input_length <= 0) {
        return;
    }

    hide_cursor(); // Hide cursor during modification

    for (int i = cursor_position - 1; i < input_length - 1; i++) {
        input_buffer[i] = input_buffer[i + 1];
    }
    
    input_length--;
    cursor_position--;
    input_buffer[input_length] = '\0';
}

void delete_char_at_cursor() {
    if (cursor_position >= input_length) {
        return;
    }

    hide_cursor(); // Hide cursor during modification

    for (int i = cursor_position; i < input_length - 1; i++) {
        input_buffer[i] = input_buffer[i + 1];
    }
    
    input_length--;
    input_buffer[input_length] = '\0';
}

void move_cursor_left() {
    if (cursor_position > 0) {
        hide_cursor();
        cursor_position--;
    }
}

void move_cursor_right() {
    if (cursor_position < input_length) {
        hide_cursor();
        cursor_position++;
    }
}

void process_command() {
    hide_cursor(); // Hide cursor during command processing
    print_newline();
    int cmd_length = 0;
    
    if (input_length > 0) {
        // Clear args array first
        for (int j = 0; j < MAX_ARGS_AMOUNT; j++) {
            for (int k = 0; k < MAX_ARGS_LENGTH; k++) {
                args[j][k] = '\0';
            }
        }
        
        int i = 0;
        // get command length
        while (i < input_length && input_buffer[i] != ' ' && 
               input_buffer[i] != '\n' && input_buffer[i] != '\r') i++;
        cmd_length = i;

        // skip the whitespaces to get to first args
        while (i < input_length && input_buffer[i] == ' ') i++; 

        int args_idx = 0;
        int args_buffer_idx = 0;
        while (i < input_length && args_idx < MAX_ARGS_AMOUNT) {
            while (i < input_length && input_buffer[i] != ' ' && 
                   input_buffer[i] != '\n' && input_buffer[i] != '\r' &&
                   args_buffer_idx < MAX_ARGS_LENGTH - 1) {
                args[args_idx][args_buffer_idx] = input_buffer[i];
                args_buffer_idx++;
                i++;
            }
            
            if (args_buffer_idx > 0) {
                args[args_idx][args_buffer_idx] = '\0'; // Null terminate
                args_buffer_idx = 0;
                args_idx++;
            }

            // skip whitespace
            while (i < input_length && input_buffer[i] == ' ') i++;
        }
        
        // do commands
        if (!memcmp("help", input_buffer, cmd_length) && cmd_length == 4) {
            help();
        }
        else if (!memcmp("clear", input_buffer, cmd_length) && cmd_length == 5) {
            clear();
        }
        else if (!memcmp("echo", input_buffer, cmd_length) && cmd_length == 4) {
            echo(args[0]);
        }
        else if (!memcmp("exit", input_buffer, cmd_length) && cmd_length == 4) {
            print_string_at_cursor("Goodbye!");
            print_newline();
            while(1) {}
        } 
        else if (!memcmp("apple", input_buffer, cmd_length) && cmd_length == 5) {
            apple(&cursor);
        } 
        else if (input_buffer[0] == 0x1B) { // ESC key
            print_string_colored("Exiting debug mode...", COLOR_LIGHT_RED);
            print_newline();
        }
        else {
            print_string_colored("Command not found: ", COLOR_LIGHT_RED);
            
            // Print command with proper null termination
            char temp_cmd[MAX_INPUT_LENGTH + 1];
            for (int j = 0; j < cmd_length && j < MAX_INPUT_LENGTH; j++) {
                temp_cmd[j] = input_buffer[j];
            }
            temp_cmd[cmd_length] = '\0';
            
            print_string_colored(temp_cmd, COLOR_WHITE);
            print_newline();
            print_string_colored("Type 'help' for available commands.", COLOR_DARK_GRAY);
            print_newline();
        }
    }
}

int main(void) {
    struct BlockBuffer      bl[10]   = {0};
    struct EXT2DriverRequest request = {
        .buf                   = &bl,
        .name                  = "shell",
        .parent_inode          = 1,
        .buffer_size           = BLOCK_SIZE * 10,
        .name_len = 5,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

    cursor.row = 0;
    cursor.col = 0;
    
    clear_input_buffer(); 
    print_prompt();
    
    syscall(7, 0, 0, 0); // activate keyboard

    char c;
    int prompt_start_col = cursor.col;
    
    // Show initial cursor
    show_cursor();
    
    while (1) {
        syscall(4, (uint32_t)&c, 0, 0);
        
        if (c != 0) {
            if (c == 0x11) { // Left arrow
                move_cursor_left();
                
                // Calculate display position
                int display_col = prompt_start_col + cursor_position;
                int display_row = cursor.row;
                
                while (display_col >= SCREEN_WIDTH) {
                    display_col -= SCREEN_WIDTH;
                    display_row++;
                }
                
                cursor.col = display_col;
                cursor.row = display_row;
                set_hardware_cursor();
                show_cursor();
                continue;
            }
            else if (c == 0x12) { // Right arrow
                move_cursor_right();
                
                // Calculate display position
                int display_col = prompt_start_col + cursor_position;
                int display_row = cursor.row;
                
                while (display_col >= SCREEN_WIDTH) {
                    display_col -= SCREEN_WIDTH;
                    display_row++;
                }
                
                cursor.col = display_col;
                cursor.row = display_row;
                set_hardware_cursor();
                show_cursor();
                continue;
            }
            else if (c == 0x10) { // Up arrow
                // TODO: Command history
                continue;
            }
            else if (c == 0x13) { // Down arrow
                // TODO: Command history
                continue;
            }
            
            if (c == '\n' || c == '\r') { // Enter key
                process_command();
                clear_input_buffer();
                print_prompt();
                prompt_start_col = cursor.col;
                show_cursor();
            } else if (c == '\b') { // Backspace
                delete_char_before_cursor();
                redraw_input_line(prompt_start_col);
            } else if (c >= ' ' && c <= '~') {
                insert_char_at_cursor(c);
                redraw_input_line(prompt_start_col);
            }
        }
        syscall(14, 0, 0, 0); 
    }
    
    return 0;
}