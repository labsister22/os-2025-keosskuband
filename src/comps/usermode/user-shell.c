#include <stdint.h>
// #include <string.h>
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"
#include "comps/usermode/commands/apple.h"
#include "header/usermode/commands/help.h"
#include "header/usermode/commands/clear.h"
#include "header/usermode/commands/echo.h"
#include "header/usermode/commands/ls.h"
#include "header/usermode/commands/cd.h"
#include "header/usermode/commands/mkdir.h"

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGS_AMOUNT 10
#define MAX_ARGS_LENGTH 32
#define SHELL_PROMPT "Keossku-Band"

#define SCREEN_WIDTH 39
#define SCREEN_HEIGHT 25

// Add cursor visibility tracking
static bool cursor_shown = false;
static char char_under_cursor = ' '; // Store character that cursor is over
static uint8_t char_color_under_cursor = COLOR_WHITE; // Store original color

CP cursor = {0, 0};
str_path path = {
    .path = "",
    .len = 0
};
absolute_dir_info DIR_INFO = {
    .current_dir = 0,
    .dir = {{".", 1, 1}}
};

char input_buffer[MAX_INPUT_LENGTH];
int input_length = 0;
char args[MAX_ARGS_AMOUNT][MAX_ARGS_LENGTH];
char command[MAX_INPUT_LENGTH + 1];
int cursor_position = 0;

static int prompt_start_row = 0;
static int prompt_start_col = 0;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

// New syscall for cursor operations
void graphics_draw_cursor_syscall() {
    syscall(10, 0, 0, 0);
}

void graphics_erase_cursor_syscall() {
    syscall(11, 0, 0, 0);
}

void graphics_store_char_syscall(char c, uint8_t color) {
    syscall(12, (uint32_t)c, (uint32_t)color, 0);
}

void graphics_set_cursor_colors_syscall(uint8_t fg_color, uint8_t bg_color) {
    syscall(15, (uint32_t)fg_color, (uint32_t)bg_color, 0);
}

void show_cursor() {
    if (!cursor_shown) {
        if (cursor_position < input_length) {
            char_under_cursor = input_buffer[cursor_position];
            char_color_under_cursor = COLOR_WHITE;
        } else {
            char_under_cursor = ' ';
            char_color_under_cursor = COLOR_BLACK;
        }

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
    syscall(8, cursor.row, cursor.col, 0);

    uint16_t pixel_x = cursor.col * 8;
    uint16_t pixel_y = cursor.row * 8;
    syscall(13, pixel_x, pixel_y, 0);
}

void scroll_screen() {
    // Simple: just move cursor to last line for now
    cursor.row = SCREEN_HEIGHT - 1;
    cursor.col = 0;
    set_hardware_cursor();
}

void advance_cursor() {
    cursor.col++;

    if (cursor.col >= SCREEN_WIDTH) {
        cursor.col = 0;
        cursor.row++;

        if (cursor.row >= SCREEN_HEIGHT) {
            scroll_screen();
        }
    }

    set_hardware_cursor();
}

void print_string_colored(const char* str, uint8_t color) {
    hide_cursor();

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
    hide_cursor();

    cursor.col = 0;
    cursor.row++;

    if (cursor.row >= SCREEN_HEIGHT) {
        scroll_screen();
    }

    set_hardware_cursor();
}

// Update cursor row and col based on cursor_position and prompt start
void update_cursor_row_col() {
    hide_cursor();
    int total_pos = prompt_start_col + cursor_position;
    cursor.row = prompt_start_row + total_pos / SCREEN_WIDTH;
    cursor.col = total_pos % SCREEN_WIDTH;

    if (cursor.row >= SCREEN_HEIGHT) {
        cursor.row = SCREEN_HEIGHT - 1;
    }
    set_hardware_cursor();
}

void print_prompt() {
    print_string_colored(SHELL_PROMPT, COLOR_LIGHT_CYAN);
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    for (int i = 0; i <= DIR_INFO.current_dir; i++) {
        for (int j = 0; j < DIR_INFO.dir[i].dir_name_len; j++) {
            syscall(5, (uint32_t)&DIR_INFO.dir[i].dir_name[j], COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
        }
        if (i < DIR_INFO.current_dir) {
            syscall(5, (uint32_t)"/", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
        }
    }

    cursor.row++;
    cursor.col = 0;
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;
    syscall(5, (uint32_t)"$", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    // Save prompt start row/col for input text (cursor after prompt)
    prompt_start_row = cursor.row;
    prompt_start_col = cursor.col;

    update_cursor_row_col();
    set_hardware_cursor();
    show_cursor();
}



void redraw_input_line() {
    hide_cursor();

    int total_chars = prompt_start_col + input_length;
    int total_rows = (total_chars + SCREEN_WIDTH - 1) / SCREEN_WIDTH;

    // Clear only input area, not prompt
    for (int row = prompt_start_row; row < prompt_start_row + total_rows && row < SCREEN_HEIGHT; row++) {
        int start_col = (row == prompt_start_row) ? prompt_start_col : 0;
        for (int col = start_col; col < SCREEN_WIDTH; col++) {
            char space = ' ';
            CP pos = {row, col};
            syscall(5, (uint32_t)&space, COLOR_BLACK, (uint32_t)&pos);
        }
    }

    // Redraw input text with wrapping
    int draw_row = prompt_start_row;
    int draw_col = prompt_start_col;
    for (int i = 0; i < input_length; i++) {
        CP pos = {draw_row, draw_col};
        syscall(5, (uint32_t)&input_buffer[i], COLOR_WHITE, (uint32_t)&pos);
        draw_col++;
        if (draw_col >= SCREEN_WIDTH) {
            draw_col = 0;
            draw_row++;
            if (draw_row >= SCREEN_HEIGHT) {
                draw_row = SCREEN_HEIGHT - 1;
            }
        }
    }

    update_cursor_row_col();
    set_hardware_cursor();
    show_cursor();
}

void insert_char_at_cursor(char c) {
    if (input_length >= MAX_INPUT_LENGTH - 1) {
        return;
    }

    hide_cursor();

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

    hide_cursor();

    for (int i = cursor_position - 1; i < input_length - 1; i++) {
        input_buffer[i] = input_buffer[i + 1];
    }

    input_length--;
    cursor_position--;
    input_buffer[input_length] = '\0';

    update_cursor_row_col();
}

void delete_char_at_cursor() {
    if (cursor_position >= input_length) {
        return;
    }

    hide_cursor();

    for (int i = cursor_position; i < input_length - 1; i++) {
        input_buffer[i] = input_buffer[i + 1];
    }

    input_length--;
    input_buffer[input_length] = '\0';

    update_cursor_row_col();
}

void move_cursor_left() {
    if (cursor_position > 0) {
        hide_cursor();
        cursor_position--;
        update_cursor_row_col();
    }
}

void move_cursor_right() {
    if (cursor_position < input_length) {
        hide_cursor();
        cursor_position++;
        update_cursor_row_col();
    }
}

void process_command() {
    hide_cursor();
    print_newline();

    if (input_length > 0) {
        // Clear args array first
        for (int j = 0; j < MAX_ARGS_AMOUNT; j++) {
            for (int k = 0; k < MAX_ARGS_LENGTH; k++) {
                args[j][k] = '\0';
            }
        }

        int i = 0;
        // get command
        while (i < input_length && input_buffer[i] != ' ' &&
               input_buffer[i] != '\n' && input_buffer[i] != '\r') {
               
           command[i] = input_buffer[i];   
           i++;
        }
        command[i] = '\0';
            

        // skip the whitespaces to get to first args
        while (i < input_length && input_buffer[i] == ' ')
            i++;

        int args_idx = 0;
        int args_buffer_idx = 0;
        int args_used_amount = 0;
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
                args_used_amount++;
            }

            // skip whitespace
            while (i < input_length && input_buffer[i] == ' ')
                i++;
        }

        // do commands
        if (!strcmp("help", command)) {
            help();
        } else if (!strcmp("clear", command)) {
            clear();
        } else if (!strcmp("echo", command)) {
            echo(args[0]);
        }
        else if (!strcmp("ls", command)) {
            ls(args[0]);
        }
        else if (!strcmp("cd", command)) {
            cd(args[0]);
        }
        else if (!strcmp("mkdir", command)) {
            mkdir(args[0]);
        }
       else if (!strcmp("exit", command)) {
            print_string_at_cursor("Goodbye!");
            print_newline();
            while (1) {
            }
        } else if (!strcmp("apple", command)) {
            apple(&cursor);
        } else if (input_buffer[0] == 0x1B) { // ESC key
            print_string_colored("Exiting debug mode...", COLOR_LIGHT_RED);
            print_newline();
        } else {
            print_string_colored("Command not found: ", COLOR_LIGHT_RED);

            print_string_colored(command, COLOR_WHITE);
            print_newline();
            print_string_colored("Type 'help' for available commands.", COLOR_DARK_GRAY);
            print_newline();
        }
    }
}

int main(void) {
    // struct BlockBuffer      bl[10]   = {0};
    // struct EXT2DriverRequest request = {
    //     .buf                   = &bl,
    //     .name                  = "shell",
    //     .parent_inode          = 1,
    //     .buffer_size           = BLOCK_SIZE * 10,
    //     .name_len = 5,
    // };
    // int32_t retcode;
    // syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

    cd_root();

    cursor.row = 0;
    cursor.col = 0;

    clear_input_buffer();
    print_prompt();

    syscall(7, 0, 0, 0); // activate keyboard

    char c;

    show_cursor();

    while (1) {
        syscall(4, (uint32_t)&c, 0, 0);

        if (c != 0) {
            if (c == 0x11) { // Left arrow
                move_cursor_left();
                set_hardware_cursor();
                show_cursor();
                continue;
            } else if (c == 0x12) { // Right arrow
                move_cursor_right();
                set_hardware_cursor();
                show_cursor();
                continue;
            } else if (c == 0x10) { // Up arrow
                // TODO: Command history
                continue;
            } else if (c == 0x13) { // Down arrow
                // TODO: Command history
                continue;
            }

            if (c == '\n' || c == '\r') { // Enter key
                process_command();
                clear_input_buffer();
                print_prompt();
                show_cursor();
            } else if (c == '\b') { // Backspace
                delete_char_before_cursor();
                redraw_input_line();
            } else if (c >= ' ' && c <= '~') {
                insert_char_at_cursor(c);
                redraw_input_line();
            }
        }
        syscall(14, 0, 0, 0);
    }

    return 0;
}
