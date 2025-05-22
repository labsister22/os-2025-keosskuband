

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

void clear_input_buffer() {
    for (int i = 0; i < MAX_INPUT_LENGTH; i++) {
        input_buffer[i] = '\0';
    }
    input_length = 0;
    cursor_position = 0;
}

void set_hardware_cursor() {
    // Note buat fariz dan jibril, aku nambahin syscall 8 buat posisi kursor
    syscall(8, cursor.row, cursor.col, 0);
}

void print_string_colored(const char* str, uint8_t color) {
    int i = 0;
    while (str[i] != '\0') {
        syscall(5, (uint32_t)&str[i], color, (uint32_t)&cursor);
        cursor.col++;

        if (cursor.col >= 80) {
            cursor.col = 0;
            cursor.row++;

            if (cursor.row >= 25) {
                cursor.row = 24;
            }
        }
        i++;
    }
    set_hardware_cursor();
}

void print_string_at_cursor(const char* str) {
    print_string_colored(str, COLOR_WHITE);
}

void print_newline() {
    cursor.col = 0;
    cursor.row++;
    if (cursor.row >= 25) {
        cursor.row = 24;
    }
    set_hardware_cursor();
}

void print_prompt() {
    int i = 0;
    while (SHELL_PROMPT[i] != '\0') {
        syscall(5, (uint32_t)&SHELL_PROMPT[i], COLOR_LIGHT_CYAN, (uint32_t)&cursor);
        cursor.col++;
        i++;
    }
    set_hardware_cursor();
}

void redraw_input_line(int prompt_start_col) {
    int current_row = cursor.row;
    cursor.col = prompt_start_col;

    for (int i = prompt_start_col; i < 80; i++) {
        char space = ' ';
        syscall(5, (uint32_t)&space, COLOR_LIGHT_GRAY, (uint32_t)&cursor);
        cursor.col++;
    }
    
    cursor.col = prompt_start_col;
    
    for (int i = 0; i < input_length; i++) {
        syscall(5, (uint32_t)&input_buffer[i], COLOR_WHITE, (uint32_t)&cursor);
        cursor.col++;
    }

    cursor.col = prompt_start_col + cursor_position;
    cursor.row = current_row;

    set_hardware_cursor();
}

void insert_char_at_cursor(char c) {
    if (input_length >= MAX_INPUT_LENGTH - 1) {
        return;
    }
    
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

    for (int i = cursor_position; i < input_length - 1; i++) {
        input_buffer[i] = input_buffer[i + 1];
    }
    
    input_length--;
    input_buffer[input_length] = '\0';
}

void move_cursor_left() {
    if (cursor_position > 0) {
        cursor_position--;
    }
}

void move_cursor_right() {
    if (cursor_position < input_length) {
        cursor_position++;
    }
}

// ya ini dummy buat ngecek, harusnya kalian kalau mau ngetes pakai memset aja - Nayaka
void process_command() {
    print_newline();
    int cmd_length = 0;
    
    
    if (input_length > 0) {
        // process input 
        
        int i = 0;
        // get command length
        while (i < input_length && input_buffer[i] != ' ' && 
               input_buffer[i] != '\n' && input_buffer[i] != '\r') i++;
        cmd_length = i;

        // skip the whitespaces to get to first args
        while (input_buffer[i] == ' ') i++; 

        int args_idx = 0;
        int args_buffer_idx = 0;
        while (i < input_length && args_idx < MAX_ARGS_AMOUNT) {

            while (i < input_length && input_buffer[i] != ' ' && 
                   input_buffer[i] != '\n' && input_buffer[i] != '\r') {
                args[args_idx][args_buffer_idx] = input_buffer[i];
                args_buffer_idx++;
                i++;
            }
            
            if (args_buffer_idx > 0) {
                args_buffer_idx = 0;
                args_idx++;
            }

            i++; // skip whitespace
        }
        

        // do commands
        if (!memcmp("help", input_buffer, cmd_length)) {
            help();
        }
        else if (!memcmp(input_buffer, "clear", cmd_length)) {
            clear();
        }
        else if (!memcmp("echo", input_buffer, cmd_length)) {
            echo(args[0]);
        }
        else if (!memcmp("exit", input_buffer, cmd_length)) {
            // gk tau implemennya gmn
            print_string_at_cursor("Goodbye!");
            print_newline();
            while(1) {}
        } else if (input_buffer[0] == 'a' && input_buffer[1] == 'p' && 
                 input_buffer[2] == 'p' && input_buffer[3] == 'l' && 
                 input_buffer[4] == 'e' && input_buffer[5] == '\0') {

            apple(&cursor);
        } else if (input_buffer[0] == 0x1B) { // ESC key
            print_string_colored("Exiting debug mode...", COLOR_LIGHT_RED);
            print_newline();
        }
        else {
            print_string_colored("Command not found: ", COLOR_LIGHT_RED);
            print_string_colored(input_buffer, COLOR_WHITE);
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
    
    while (1) {
        syscall(4, (uint32_t)&c, 0, 0);
        
        if (c != 0) {
            if (c == 0x11) { // Left arrow (0x4B -> 0x11)
                move_cursor_left();
                cursor.col = prompt_start_col + cursor_position;
                set_hardware_cursor();
                continue;
            }
            else if (c == 0x12) { // Right arrow (0x4D -> 0x12)
                move_cursor_right();
                cursor.col = prompt_start_col + cursor_position;
                set_hardware_cursor();
                continue;
            }
            else if (c == 0x10) { // Up arrow (0x48 -> 0x10)
                // TODO: Implement i guess?
                continue;
            }
            else if (c == 0x13) { // Down arrow (0x50 -> 0x13)
                // TODO: What?
                continue;
            }
            
            if (c == '\n' || c == '\r') { // Enter key
                print_newline();
                process_command();
                clear_input_buffer();
                print_prompt();
                prompt_start_col = cursor.col;
            } else if (c == '\b') { // Backspace
                delete_char_before_cursor();
                redraw_input_line(prompt_start_col);
            } else if (c >= ' ' && c <= '~') {
                insert_char_at_cursor(c);
                redraw_input_line(prompt_start_col);
            }
        }
    }
    
    return 0;
}