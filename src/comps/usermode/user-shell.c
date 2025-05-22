#include <stdint.h>
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"
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
        }
        else {
            print_string_at_cursor("Command not found: ");
            print_string_at_cursor(input_buffer);
            print_newline();
            print_string_at_cursor("Type 'help' for available commands.");
            print_newline();
        }
    }
}

int main(void) {
    cursor.row = 0;
    cursor.col = 0;
    
    clear_input_buffer(); 
    print_prompt();
    
    syscall(7, 0, 0, 0); // activate keyboard

    char c;
    while (true) {
        syscall(4, (uint32_t)&c, 0, 0);
        
        if (c != 0) {
            if (c == '\n' || c == '\r') {
                print_newline();
                process_command();
                clear_input_buffer();
                print_prompt();
            }

            else if (c == '\b') {
                if (input_length > 0) {
                    input_length--;
                    input_buffer[input_length] = '\0';
                    
                    if (cursor.col > 0) {
                        cursor.col--;
                    } else if (cursor.row > 0) {
                        cursor.row--;
                        cursor.col = 79;
                    }
                    
                    char space = ' ';
                    syscall(5, (uint32_t)&space, COLOR_LIGHT_GRAY, (uint32_t)&cursor);
                    set_hardware_cursor();
                }
            }

            else if (c >= ' ' && c <= '~') {
                if (input_length < MAX_INPUT_LENGTH - 1) {
                    input_buffer[input_length] = c;
                    input_length++;
                    input_buffer[input_length] = '\0';
                    
                    syscall(5, (uint32_t)&c, COLOR_WHITE, (uint32_t)&cursor);
                    cursor.col++;
                    
                    if (cursor.col >= 80) {
                        cursor.col = 0;
                        cursor.row++;
                        if (cursor.row >= 25) {
                            cursor.row = 24;
                        }
                    }
                    set_hardware_cursor();
                }
            }
        }
    }
    
    return 0;
}