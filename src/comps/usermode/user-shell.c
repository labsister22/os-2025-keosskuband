#include <stdint.h>
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"

#define MAX_INPUT_LENGTH 256
#define SHELL_PROMPT "Keossku-Band$/ "

typedef struct {
    int32_t row;
    int32_t col;
} CP;

CP cursor = {0, 0};
char input_buffer[MAX_INPUT_LENGTH];
int input_length = 0;

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
    
    if (input_length > 0) {
        if (input_buffer[0] == 'h' && input_buffer[1] == 'e' && 
            input_buffer[2] == 'l' && input_buffer[3] == 'p' && 
            input_buffer[4] == '\0') {
            
            print_string_at_cursor("Available commands:");
            print_newline();
            print_string_at_cursor("  help - Show this help message");
            print_newline();
            print_string_at_cursor("  clear - Clear the screen");
            print_newline();
            print_string_at_cursor("  echo [text] - Echo the text");
            print_newline();
            print_string_at_cursor("  exit - Exit the shell");
            print_newline();
        }
        else if (input_buffer[0] == 'c' && input_buffer[1] == 'l' && 
                 input_buffer[2] == 'e' && input_buffer[3] == 'a' && 
                 input_buffer[4] == 'r' && input_buffer[5] == '\0') {
            
            cursor.row = 0;
            cursor.col = 0;
            set_hardware_cursor();
            
            for (int i = 0; i < 25; i++) {
                for (int j = 0; j < 80; j++) {
                    CP temp = {i, j};
                    char space = ' ';
                    syscall(5, (uint32_t)&space, 0x07, (uint32_t)&temp);
                }
            }
            cursor.row = 0;
            cursor.col = 0;
        }
        else if (input_buffer[0] == 'e' && input_buffer[1] == 'c' && 
                 input_buffer[2] == 'h' && input_buffer[3] == 'o' && 
                 input_buffer[4] == ' ') {

            print_string_at_cursor(&input_buffer[5]);
            print_newline();
        }
        else if (input_buffer[0] == 'e' && input_buffer[1] == 'x' && 
                 input_buffer[2] == 'i' && input_buffer[3] == 't' && 
                 input_buffer[4] == '\0') {
            
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
    
    syscall(7, 0, 0, 0);

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