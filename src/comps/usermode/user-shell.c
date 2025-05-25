#include <stdint.h>
#include <stdbool.h>
#include "header/filesys/ext2.h"
#include "header/stdlib/strops.h"
#include "header/usermode/user-shell.h"
#include "header/usermode/commands/apple.h"
#include "header/usermode/commands/help.h"
#include "header/usermode/commands/clear.h"
#include "header/usermode/commands/echo.h"
#include "header/usermode/commands/ls.h"
#include "header/usermode/commands/cd.h"
#include "header/usermode/commands/mkdir.h"
#include "header/usermode/commands/find.h"
#include "header/usermode/commands/cat.h"

static CommandHistory history = {
    .count = 0,
    .current_index = -1
};

static ShellState shell_state = {
    .input_length = 0,
    .cursor_position = 0,
    .prompt_start_row = 0,
    .prompt_start_col = 0,
    .cursor_shown = false,
    .char_under_cursor = ' ',
    .char_color_under_cursor = COLOR_WHITE
};

CP cursor = {0, 0};
str_path path = {
    .path = "",
    .len = 0
};
absolute_dir_info DIR_INFO = {
    .current_dir = 0,
    .dir = {{".", 1, 1}}
};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

void add_to_history(const char* command) {
    if (strlen((char*)command) == 0) {
        return;
    }

    if (history.count > 0 && 
        strcmp(history.commands[(history.count - 1) % MAX_HISTORY_ENTRIES], (char*)command) == 0) {
        return;
    }

    int index = history.count % MAX_HISTORY_ENTRIES;
    strcpy(history.commands[index], (char*)command);
    
    if (history.count < MAX_HISTORY_ENTRIES) {
        history.count++;
    }

    history.current_index = -1;
}

void load_history_entry(int index) {
    if (index < 0 || index >= history.count) {
        return;
    }

    int actual_index;
    if (history.count < MAX_HISTORY_ENTRIES) {
        actual_index = index;
    } else {
        actual_index = (history.count + index) % MAX_HISTORY_ENTRIES;
    }
    
    clear_input_buffer();

    strcpy(shell_state.input_buffer, history.commands[actual_index]);
    shell_state.input_length = strlen(shell_state.input_buffer);
    shell_state.cursor_position = shell_state.input_length;
    
    redraw_input_line();
}

void handle_up_arrow() {
    if (history.count == 0) {
        return;
    }
    
    if (history.current_index == -1) {
        history.current_index = history.count - 1;
    } else if (history.current_index > 0) {
        history.current_index--;
    }
    
    load_history_entry(history.current_index);
}

void handle_down_arrow() {
    if (history.count == 0 || history.current_index == -1) {
        return;
    }
    
    if (history.current_index < history.count - 1) {
        history.current_index++;
        load_history_entry(history.current_index);
    } else {
        history.current_index = -1;
        clear_input_buffer();
        redraw_input_line();
    }
}

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
    if (!shell_state.cursor_shown) {
        if (shell_state.cursor_position < shell_state.input_length) {
            shell_state.char_under_cursor = shell_state.input_buffer[shell_state.cursor_position];
            shell_state.char_color_under_cursor = COLOR_WHITE;
        } else {
            shell_state.char_under_cursor = ' ';
            shell_state.char_color_under_cursor = COLOR_BLACK;
        }

        graphics_set_cursor_colors_syscall(COLOR_WHITE, COLOR_GREEN);
        graphics_store_char_syscall(shell_state.char_under_cursor, shell_state.char_color_under_cursor);
        graphics_draw_cursor_syscall();
        shell_state.cursor_shown = true;
    }
}

void hide_cursor() {
    if (shell_state.cursor_shown) {
        graphics_erase_cursor_syscall();
        shell_state.cursor_shown = false;
    }
}

void clear_input_buffer() {
    for (int i = 0; i < MAX_INPUT_LENGTH; i++) {
        shell_state.input_buffer[i] = '\0';
    }
    shell_state.input_length = 0;
    shell_state.cursor_position = 0;
}

void set_hardware_cursor() {
    syscall(8, cursor.row, cursor.col, 0);

    uint16_t pixel_x = cursor.col * 5;
    uint16_t pixel_y = cursor.row * 8;
    syscall(13, pixel_x, pixel_y, 0);
}

void scroll_screen() {
    syscall(18, 8, COLOR_BLACK, 0); // Scroll 1 line up
    set_hardware_cursor();
}

void check_and_scroll() {
    while (cursor.row >= SCREEN_HEIGHT) {
        scroll_screen();
        if (shell_state.prompt_start_row > 0) {
            shell_state.prompt_start_row--;
            cursor.row--;
        }
    }
}

void advance_cursor() {
    cursor.col++;

    if (cursor.col >= SCREEN_WIDTH) {
        cursor.col = 0;
        cursor.row++;
    }

    check_and_scroll();
    set_hardware_cursor();
}

void print_string_colored(const char* str, uint8_t color) {
    hide_cursor();

    int i = 0;
    while (str[i] != '\0') {
        check_and_scroll();
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

    check_and_scroll();
    set_hardware_cursor();
}

// Update cursor row and col based on cursor_position and prompt start
void update_cursor_row_col() {
    hide_cursor();
    int total_pos = shell_state.prompt_start_col + shell_state.cursor_position;
    cursor.row = shell_state.prompt_start_row + total_pos / SCREEN_WIDTH;
    cursor.col = total_pos % SCREEN_WIDTH;

    // Check if we need to scroll due to cursor position
    check_and_scroll();
    set_hardware_cursor();
}

void print_prompt() {
    // Check if we need to scroll before printing prompt
    check_and_scroll();
    
    print_string_colored(SHELL_PROMPT, COLOR_LIGHT_CYAN);
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    for (int i = 0; i <= DIR_INFO.current_dir; i++) {
        for (int j = 0; j < DIR_INFO.dir[i].dir_name_len; j++) {
            syscall(5, (uint32_t)&DIR_INFO.dir[i].dir_name[j], COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
            if (cursor.col >= SCREEN_WIDTH) {
                cursor.col = 0;
                cursor.row++;
                check_and_scroll();
            }
        }
        if (i < DIR_INFO.current_dir) {
            syscall(5, (uint32_t)"/", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
            if (cursor.col >= SCREEN_WIDTH) {
                cursor.col = 0;
                cursor.row++;
                check_and_scroll();
            }
        }
    }

    cursor.col = 0;
    cursor.row++;
    check_and_scroll();
    
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;
    syscall(5, (uint32_t)"$", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    // Save prompt start row/col for input text (cursor after prompt)
    shell_state.prompt_start_row = cursor.row;
    shell_state.prompt_start_col = cursor.col;

    update_cursor_row_col();
    set_hardware_cursor();
    show_cursor();
}

void redraw_input_line() {
    hide_cursor();

    // Calculate how many rows the input will take
    int total_chars = shell_state.prompt_start_col + shell_state.input_length;
    int total_rows = (total_chars + SCREEN_WIDTH - 1) / SCREEN_WIDTH;
    int end_row = shell_state.prompt_start_row + total_rows - 1;

    // Check if input would extend beyond screen and scroll if needed
    while (end_row >= SCREEN_HEIGHT) {
        scroll_screen();
        shell_state.prompt_start_row--;
        end_row--;
    }

    // Clear only input area, not prompt
    for (int row = shell_state.prompt_start_row; row <= end_row && row < SCREEN_HEIGHT; row++) {
        int start_col = (row == shell_state.prompt_start_row) ? shell_state.prompt_start_col : 0;
        for (int col = start_col; col < SCREEN_WIDTH; col++) {
            char space = ' ';
            CP pos = {row, col};
            syscall(5, (uint32_t)&space, COLOR_BLACK, (uint32_t)&pos);
        }
    }

    // Redraw input text with wrapping
    int draw_row = shell_state.prompt_start_row;
    int draw_col = shell_state.prompt_start_col;
    for (int i = 0; i < shell_state.input_length; i++) {
        CP pos = {draw_row, draw_col};
        syscall(5, (uint32_t)&shell_state.input_buffer[i], COLOR_WHITE, (uint32_t)&pos);
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
    if (shell_state.input_length >= MAX_INPUT_LENGTH - 1) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.input_length; i > shell_state.cursor_position; i--) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i - 1];
    }

    shell_state.input_buffer[shell_state.cursor_position] = c;
    shell_state.input_length++;
    shell_state.cursor_position++;
    shell_state.input_buffer[shell_state.input_length] = '\0';
}

void delete_char_before_cursor() {
    if (shell_state.cursor_position <= 0 || shell_state.input_length <= 0) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.cursor_position - 1; i < shell_state.input_length - 1; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + 1];
    }

    shell_state.input_length--;
    shell_state.cursor_position--;
    shell_state.input_buffer[shell_state.input_length] = '\0';

    update_cursor_row_col();
}

void delete_char_at_cursor() {
    if (shell_state.cursor_position >= shell_state.input_length) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.cursor_position; i < shell_state.input_length - 1; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + 1];
    }

    shell_state.input_length--;
    shell_state.input_buffer[shell_state.input_length] = '\0';

    update_cursor_row_col();
}

void move_cursor_left() {
    if (shell_state.cursor_position > 0) {
        hide_cursor();
        shell_state.cursor_position--;
        update_cursor_row_col();
    }
}

void move_cursor_right() {
    if (shell_state.cursor_position < shell_state.input_length) {
        hide_cursor();
        shell_state.cursor_position++;
        update_cursor_row_col();
    }
}

void process_command() {
    hide_cursor();
    print_newline();

    if (shell_state.input_length > 0) {
        add_to_history(shell_state.input_buffer);
        
        for (int j = 0; j < MAX_ARGS_AMOUNT; j++) {
            for (int k = 0; k < MAX_ARGS_LENGTH; k++) {
                shell_state.args[j][k] = '\0';
            }
        }

        int i = 0;
        // get command
        while (i < shell_state.input_length && shell_state.input_buffer[i] != ' ' &&
               shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r') {
               
           shell_state.command[i] = shell_state.input_buffer[i];   
           i++;
        }
        shell_state.command[i] = '\0';
            

        // skip the whitespaces to get to first args
        while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
            i++;

        int args_idx = 0;
        int args_buffer_idx = 0;
        int args_used_amount = 0;
        while (i < shell_state.input_length && args_idx < MAX_ARGS_AMOUNT) {
            while (i < shell_state.input_length && shell_state.input_buffer[i] != ' ' &&
                   shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r' &&
                   args_buffer_idx < MAX_ARGS_LENGTH - 1) {
                shell_state.args[args_idx][args_buffer_idx] = shell_state.input_buffer[i];
                args_buffer_idx++;
                i++;
            }

            if (args_buffer_idx > 0) {
                shell_state.args[args_idx][args_buffer_idx] = '\0'; // Null terminate
                args_buffer_idx = 0;
                args_idx++;
                args_used_amount++;
            }

            // skip whitespace
            while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
                i++;
        }

        // do commands using strcmp from strops.h
        if (strcmp("help", shell_state.command) == 0) {
            help();
        } else if (strcmp("clear", shell_state.command) == 0) {
            clear();
        } else if (strcmp("echo", shell_state.command) == 0) {
            echo(shell_state.args[0]);
        }
        else if (strcmp("ls", shell_state.command) == 0) {
            ls(shell_state.args[0]);
        }
        else if (strcmp("cd", shell_state.command) == 0) {
            cd(shell_state.args[0]);
        }
        else if (strcmp("mkdir", shell_state.command) == 0) {
            mkdir(shell_state.args[0]);
        }
        else if (strcmp("find", shell_state.command) == 0) {
            if (args_used_amount > 0) {
                find(shell_state.args[0]);
            } else {
                print_string_colored("Usage: find <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (!strcmp("cat", shell_state.command)) {
            if (args_used_amount > 0) {
                cat(shell_state.args[0]);
            } else {
                print_string_colored("Usage: cat <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (!strcmp("cls", shell_state.command)) {
            clear_input_buffer();
            print_prompt();
        } else if (strcmp("reboot", shell_state.command) == 0) {
            print_string_at_cursor("Rebooting...");
            print_newline();
            syscall(2, 0, 0, 0); // Reboot syscall
        } else if (strcmp("shutdown", shell_state.command) == 0) {
            print_string_at_cursor("Shutting down...");
            print_newline();
            syscall(3, 0, 0, 0); // Shutdown syscall
        }
        else if (strcmp("exit", shell_state.command) == 0) {
            print_string_at_cursor("Goodbye!");
            print_newline();
            while (1) {
            }
        } else if (strcmp("apple", shell_state.command) == 0) {
            apple(&cursor);
        } else if (shell_state.input_buffer[0] == 0x1B) { // ESC key
            print_string_colored("Exiting debug mode...", COLOR_LIGHT_RED);
            print_newline();
        } else {
            print_string_colored("Command not found: ", COLOR_LIGHT_RED);

            print_string_colored(shell_state.command, COLOR_WHITE);
            print_newline();
            print_string_colored("Type 'help' for available commands.", COLOR_DARK_GRAY);
            print_newline();
        }
    }
}

int main(void) {
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
            } else if (c == 0x10) { // Up arrow - Navigate to previous command
                handle_up_arrow();
                continue;
            } else if (c == 0x13) { // Down arrow - Navigate to next command
                handle_down_arrow();
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
        
        // Additional scroll check in main loop
        check_and_scroll();
        syscall(14, 0, 0, 0);
    }

    return 0;
}