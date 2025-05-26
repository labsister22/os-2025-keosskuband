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
#include "header/usermode/commands/touch.h"
#include "header/usermode/commands/ps.h"
#include "header/usermode/commands/kill.h"
#include "header/usermode/commands/exec.h"
#include "header/usermode/commands/ikuyokita.h"
#include "header/usermode/commands/rm.h"
#include "header/usermode/commands/cp.h"
#include "header/usermode/commands/mv.h"

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

// Tab completion state
static CompletionState completion_state = {
    .count = 0,
    .current_selection = 0,
    .is_showing = false
};

// Command list for completion
static const char* command_list[] = {
    "help", "clear", "echo", "ls", "cd", "mkdir", "find", "cat", 
    "cls", "exit", "apple", NULL
};

// Store the original prefix for cycling
static char original_prefix[MAX_COMPLETION_LENGTH] = "";
static int original_prefix_len = 0;
static int word_start_pos = 0;

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

// Helper function to check if string starts with prefix
bool starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    
    int prefix_len = strlen((char*)prefix);
    int str_len = strlen((char*)str);
    
    if (prefix_len > str_len) return false;
    
    for (int i = 0; i < prefix_len; i++) {
        if (str[i] != prefix[i]) return false;
    }
    return true;
}

// Find command completions
void find_command_completions(const char* prefix) {
    completion_state.count = 0;
    
    for (int i = 0; command_list[i] != NULL && completion_state.count < MAX_COMPLETIONS; i++) {
        if (starts_with(command_list[i], prefix)) {
            strcpy(completion_state.completions[completion_state.count], (char*)command_list[i]);
            completion_state.count++;
        }
    }
}

// Find file/directory completions
void find_file_completions(const char* command, const char* prefix) {
    completion_state.count = 0;
    
    // Read current directory contents
    uint8_t dir_buffer[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_buffer,
        .name = ".",
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buffer_size = BLOCK_SIZE,
        .name_len = 1,
        .is_directory = false
    };
    
    int8_t result;
    syscall(1, (uint32_t)&request, (uint32_t)&result, 0); // read_directory syscall
    
    if (result != 0) return;
    
    // Parse directory entries
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE && completion_state.count < MAX_COMPLETIONS) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry*)(dir_buffer + offset);
        
        if (entry->rec_len == 0) break;
        if (entry->inode == 0) {
            offset += entry->rec_len;
            continue;
        }
        
        // Create null-terminated name
        char entry_name[256];
        for (int i = 0; i < entry->name_len && i < 255; i++) {
            entry_name[i] = entry->name[i];
        }
        entry_name[entry->name_len] = '\0';
        
        // Skip . and .. for most commands
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            if (strcmp((char*)command, "cd") != 0) {
                offset += entry->rec_len;
                continue;
            }
        }
        
        // Filter based on command type
        bool include = false;
        if (strcmp((char*)command, "cd") == 0) {
            // Only directories for cd
            include = (entry->file_type == EXT2_FT_DIR);
        } else if (strcmp((char*)command, "cat") == 0) {
            // Only regular files for cat
            include = (entry->file_type == EXT2_FT_REG_FILE);
        } else {
            // All entries for other commands
            include = true;
        }
        
        if (include && starts_with(entry_name, prefix)) {
            strcpy(completion_state.completions[completion_state.count], entry_name);
            completion_state.count++;
        }
        
        offset += entry->rec_len;
    }
}

// Apply current completion from the queue
void apply_current_completion() {
    if (completion_state.count == 0) return;
    
    hide_cursor();
    
    // Get current completion
    const char* completion = completion_state.completions[completion_state.current_selection];
    int completion_len = strlen((char*)completion);
    
    // Remove current word (from word_start_pos to cursor)
    int current_word_len = shell_state.cursor_position - word_start_pos;
    for (int i = word_start_pos; i < shell_state.input_length - current_word_len; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + current_word_len];
    }
    shell_state.input_length -= current_word_len;
    shell_state.cursor_position = word_start_pos;
    
    // Insert the completion
    if (shell_state.input_length + completion_len < MAX_INPUT_LENGTH - 1) {
        // Make room for the completion
        for (int i = shell_state.input_length + completion_len - 1; i >= word_start_pos + completion_len; i--) {
            shell_state.input_buffer[i] = shell_state.input_buffer[i - completion_len];
        }
        
        // Insert the completion
        for (int i = 0; i < completion_len; i++) {
            shell_state.input_buffer[word_start_pos + i] = completion[i];
        }
        
        shell_state.input_length += completion_len;
        shell_state.cursor_position += completion_len;
        shell_state.input_buffer[shell_state.input_length] = '\0';
        
        // Add space after completion if we're at the end
        if (shell_state.cursor_position == shell_state.input_length && 
            shell_state.input_length < MAX_INPUT_LENGTH - 1) {
            shell_state.input_buffer[shell_state.input_length] = ' ';
            shell_state.input_length++;
            shell_state.cursor_position++;
            shell_state.input_buffer[shell_state.input_length] = '\0';
        }
    }
    
    redraw_input_line();
}

// Reset completion state
void reset_completion_state() {
    completion_state.count = 0;
    completion_state.current_selection = 0;
    completion_state.is_showing = false;
    original_prefix[0] = '\0';
    original_prefix_len = 0;
}

// Handle tab completion with cycling
void handle_tab_completion() {
    // If we're already in completion mode, cycle to next completion
    if (completion_state.is_showing && completion_state.count > 0) {
        completion_state.current_selection = (completion_state.current_selection + 1) % completion_state.count;
        apply_current_completion();
        return;
    }
    
    // Start new completion
    reset_completion_state();
    
    if (shell_state.input_length == 0) {
        // No input - complete with first command
        find_command_completions("");
        if (completion_state.count > 0) {
            word_start_pos = 0;
            strcpy(original_prefix, "");
            original_prefix_len = 0;
            completion_state.is_showing = true;
            apply_current_completion();
        }
        return;
    }
    
    // Parse current input to determine context
    char temp_input[MAX_INPUT_LENGTH];
    strcpy(temp_input, shell_state.input_buffer);
    
    // Find command end
    int command_end = 0;
    while (command_end < shell_state.input_length && temp_input[command_end] != ' ') {
        command_end++;
    }
    
    if (shell_state.cursor_position <= command_end) {
        // Cursor is in command part - complete commands
        word_start_pos = 0;
        temp_input[command_end] = '\0';
        strcpy(original_prefix, temp_input);
        original_prefix_len = strlen(original_prefix);
        find_command_completions(temp_input);
    } else {
        // Cursor is in arguments part - complete files/directories
        temp_input[command_end] = '\0';
        char* command = temp_input;
        
        // Find current word start
        word_start_pos = shell_state.cursor_position;
        while (word_start_pos > 0 && shell_state.input_buffer[word_start_pos - 1] != ' ') {
            word_start_pos--;
        }
        
        // Get current argument prefix
        char arg_prefix[MAX_INPUT_LENGTH];
        int prefix_len = shell_state.cursor_position - word_start_pos;
        if (prefix_len > 0) {
            for (int j = 0; j < prefix_len; j++) {
                arg_prefix[j] = shell_state.input_buffer[word_start_pos + j];
            }
        }
        arg_prefix[prefix_len] = '\0';
        
        strcpy(original_prefix, arg_prefix);
        original_prefix_len = strlen(original_prefix);
        find_file_completions(command, arg_prefix);
    }
    
    // Apply first completion if any found
    if (completion_state.count > 0) {
        completion_state.is_showing = true;
        apply_current_completion();
    }
}

void add_to_history(const char* command) {
    if (strlen((char*)command) == 0) {
        return;
    }

    if (history.count > 0) {
        int most_recent_index;
        if (history.count < MAX_HISTORY_ENTRIES) {
            most_recent_index = history.count - 1;
        } else {
            most_recent_index = (history.count - 1) % MAX_HISTORY_ENTRIES;
        }
        
        if (strcmp(history.commands[most_recent_index], (char*)command) == 0) {
            return;
        }
    }

    if (history.count < MAX_HISTORY_ENTRIES) {
        strcpy(history.commands[history.count], (char*)command);
        history.count++;
    } else {
        for (int i = 0; i < MAX_HISTORY_ENTRIES - 1; i++) {
            strcpy(history.commands[i], history.commands[i + 1]);
        }

        strcpy(history.commands[MAX_HISTORY_ENTRIES - 1], (char*)command);
    }

    history.current_index = -1;
}

void load_history_entry(int index) {
    if (index < 0 || index >= history.count) {
        return;
    }

    clear_input_buffer();

    strcpy(shell_state.input_buffer, history.commands[index]);
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
    } else {
        return;
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
        // Make sure cursor screen position is up to date
        update_cursor_row_col();
        
        // Store the character that's currently at the cursor position
        if (shell_state.cursor_position < shell_state.input_length) {
            shell_state.char_under_cursor = shell_state.input_buffer[shell_state.cursor_position];
            shell_state.char_color_under_cursor = COLOR_WHITE;
        } else {
            shell_state.char_under_cursor = ' ';
            shell_state.char_color_under_cursor = COLOR_BLACK;
        }

        // Set cursor colors and draw
        graphics_set_cursor_colors_syscall(COLOR_WHITE, COLOR_GREEN);
        graphics_store_char_syscall(shell_state.char_under_cursor, shell_state.char_color_under_cursor);
        graphics_draw_cursor_syscall();
        shell_state.cursor_shown = true;
    }
}

void hide_cursor() {
    if (shell_state.cursor_shown) {
        graphics_erase_cursor_syscall();
        
        // Calculate the correct screen position for this logical cursor position
        int total_pos = shell_state.prompt_start_col + shell_state.cursor_position;
        int screen_row = shell_state.prompt_start_row + total_pos / SCREEN_WIDTH;
        int screen_col = total_pos % SCREEN_WIDTH;
        
        // Restore the correct character at the calculated position
        char char_to_restore;
        uint8_t color_to_restore;
        
        if (shell_state.cursor_position < shell_state.input_length) {
            char_to_restore = shell_state.input_buffer[shell_state.cursor_position];
            color_to_restore = COLOR_WHITE;
        } else {
            char_to_restore = ' ';
            color_to_restore = COLOR_BLACK;
        }
        
        CP pos = {screen_row, screen_col};
        syscall(5, (uint32_t)&char_to_restore, color_to_restore, (uint32_t)&pos);
        
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
    uint16_t pixel_x = cursor.col * 5;
    uint16_t pixel_y = cursor.row * 8;
    syscall(8, pixel_x, pixel_y, 0);
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
        }
        cursor.row--;
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

void update_cursor_row_col() {
    hide_cursor();
    int total_pos = shell_state.prompt_start_col + shell_state.cursor_position;
    cursor.row = shell_state.prompt_start_row + total_pos / SCREEN_WIDTH;
    cursor.col = total_pos % SCREEN_WIDTH;

    check_and_scroll();
    set_hardware_cursor();
}

void print_prompt() {
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

    print_newline();
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

    int total_chars = shell_state.prompt_start_col + shell_state.input_length;
    int total_rows = (total_chars + SCREEN_WIDTH - 1) / SCREEN_WIDTH;
    int end_row = shell_state.prompt_start_row + total_rows - 1;

    while (end_row >= SCREEN_HEIGHT) {
        scroll_screen();
        shell_state.prompt_start_row--;
        end_row--;
    }

    for (int row = shell_state.prompt_start_row; row <= end_row && row < SCREEN_HEIGHT; row++) {
        int start_col = (row == shell_state.prompt_start_row) ? shell_state.prompt_start_col : 0;
        for (int col = start_col; col < SCREEN_WIDTH; col++) {
            char space = ' ';
            CP pos = {row, col};
            syscall(5, (uint32_t)&space, COLOR_BLACK, (uint32_t)&pos);
        }
    }

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
        shell_state.cursor_position--;
        redraw_input_line();
    }
}

void move_cursor_right() {
    if (shell_state.cursor_position < shell_state.input_length) {
        shell_state.cursor_position++;
        redraw_input_line();
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

        if (strcmp(shell_state.command, "echo") == 0) {
            while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
                i++;

            int text_start = i;
            int text_length = 0;
            //find the args to be "echo-ed"
            while (i < shell_state.input_length && shell_state.input_buffer[i] != '|' &&
                shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r') {
                text_length++;
                i++;
            }

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

            if (args_used_amount == 0) {
                echo(shell_state.input_buffer + text_start, text_length);
            } else if (args_used_amount == 3 && 
                       strcmp(shell_state.args[0], "|") == 0 &&
                       strcmp(shell_state.args[1], "touch") == 0 ) {
                // Special case for echo to file
                shell_state.input_buffer[text_start + text_length] = '\0'; // Null terminate the echo text
                touch(shell_state.args[2], shell_state.input_buffer + text_start, text_length);
            } else {
                print_string_colored("Usage: [echo <text>] or [echo <text> | touch <filename>]", COLOR_LIGHT_RED);
                print_newline();
            }

            return;
        }
            

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
        } else if (strcmp("ls", shell_state.command) == 0) {
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
        } else if (!strcmp("touch", shell_state.command)) {
            if (args_used_amount > 0) {
                touch(shell_state.args[0], NULL, 0);
            } else {
                print_string_colored("Usage: touch <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (strcmp("show_color", shell_state.command) == 0) {
            syscall(19, 0, 0, 0);
        } else if (strcmp("rm", shell_state.command) == 0) {
            rm(shell_state.args[0], shell_state.args[1], shell_state.args[2]);
        } else if (strcmp("cp", shell_state.command) == 0) {
            cp(shell_state.args[0], shell_state.args[1], shell_state.args[2], shell_state.args[3]);
        } else if (strcmp("mv", shell_state.command) == 0) {
            mv(shell_state.args[0], shell_state.args[1], shell_state.args[2]);
        } else if (strcmp("cls", shell_state.command) == 0) {
            clear();
        }
        else if (strcmp("exit", shell_state.command) == 0) {
            print_string_at_cursor("Goodbye!");
            print_newline();
            while (1) {}
        } else if (strcmp("apple", shell_state.command) == 0) {
            apple(&cursor);
        } else if (strcmp("ikuyokita", shell_state.command) == 0) {
            ikuyokita();
        } else if (strcmp("clock", shell_state.command) == 0) {
            exec("clock", DIR_INFO.dir[DIR_INFO.current_dir].inode);
        } else if (strcmp("ps", shell_state.command) == 0) {
            ps();
        } else if (strcmp("kill", shell_state.command) == 0) {
            kill(shell_state.args[0]);
        } else if (strcmp("exec", shell_state.command) == 0) {
            exec(shell_state.args[0], DIR_INFO.dir[DIR_INFO.current_dir].inode);
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
        // syscall(14, 0, 0, 0);

        if (c != 0) {
            // Reset completion state on any key except tab
            if (c != '\t') {
                reset_completion_state();
            }
            
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
            } else if (c == '\t') { // Tab key - Cycle through completions
                handle_tab_completion();
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
        
        check_and_scroll();
    }

    return 0;
}