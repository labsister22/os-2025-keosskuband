#ifndef _USER_SHELL
#define _USER_SHELL

#include <stdint.h>
#include <stdbool.h>
#include "header/filesys/ext2.h"

#define BLOCK_COUNT 16
#define MAX_COMPLETIONS 50
#define MAX_COMPLETION_LENGTH 64

#define COLOR_BLACK       0x00
#define COLOR_BLUE        0x01
#define COLOR_GREEN       0x02
#define COLOR_CYAN        0x03
#define COLOR_RED         0x04
#define COLOR_MAGENTA     0x05
#define COLOR_BROWN       0x06
#define COLOR_LIGHT_GRAY  0x07
#define COLOR_DARK_GRAY   0x08
#define COLOR_LIGHT_BLUE  0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN  0x0B
#define COLOR_LIGHT_RED   0x0C
#define COLOR_PINK        0x0D
#define COLOR_YELLOW      0x0E
#define COLOR_WHITE       0x0F

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGS_AMOUNT 10
#define MAX_ARGS_LENGTH 32
#define MAX_PATH_LENGTH 256
#define SHELL_PROMPT "Keossku-Band"
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 25
#define MAX_HISTORY_ENTRIES 20

typedef struct {
    int32_t row;
    int32_t col;
} CP;

typedef struct {
    char path[2048];
    int len;
} str_path;

typedef struct {
    char dir_name[12];
    uint8_t dir_name_len;
    uint32_t inode;
} dir_info; 

typedef struct {
    int current_dir;
    dir_info dir[50];
} absolute_dir_info;

typedef enum {
    COMPLETION_COMMAND,
    COMPLETION_FLAG,
    COMPLETION_FILE,
    COMPLETION_DIRECTORY,
    COMPLETION_PATH,
    COMPLETION_PIPELINE,
    COMPLETION_RELATIVE_PATH
} completion_context_t;

typedef struct {
    completion_context_t context;
    char command[MAX_ARGS_LENGTH];
    char current_word[MAX_COMPLETION_LENGTH];
    char path_prefix[MAX_PATH_LENGTH];
    int word_start;
    bool after_pipeline;
    bool is_flag;
    bool is_path;
} completion_context;

// Completion structures
typedef struct {
    char completions[MAX_COMPLETIONS][MAX_COMPLETION_LENGTH];
    int count;
    int current_selection;
    bool is_showing;
} CompletionState;

// Command history structure
typedef struct {
    char commands[MAX_HISTORY_ENTRIES][MAX_INPUT_LENGTH];
    int count;
    int current_index;
} CommandHistory;

// Shell state structure
typedef struct {
    char input_buffer[MAX_INPUT_LENGTH];
    int input_length;
    char args[MAX_ARGS_AMOUNT][MAX_ARGS_LENGTH];
    char command[MAX_INPUT_LENGTH + 1];
    int cursor_position;
    int prompt_start_row;
    int prompt_start_col;
    bool cursor_shown;
    char char_under_cursor;
    uint8_t char_color_under_cursor;
} ShellState;

extern CP cursor;
extern str_path path;
extern absolute_dir_info DIR_INFO;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Output commands
void print_string_at_cursor(const char* str);
void print_string_colored(const char* str, uint8_t color);
void print_newline();
void set_hardware_cursor();

// History functions
void add_to_history(const char* command);
void load_history_entry(int index);
void handle_up_arrow();
void handle_down_arrow();

// Misc utility functions
void update_cursor_row_col();
void print_prompt();

// Input buffer and display functions
void clear_input_buffer();
void redraw_input_line();
void show_cursor();
void hide_cursor();

// Cursor movement and text editing
void insert_char_at_cursor(char c);
void delete_char_before_cursor();
void delete_char_at_cursor();
void move_cursor_left();
void move_cursor_right();

// Screen management
void scroll_screen();
void check_and_scroll();
void advance_cursor();

// Graphics cursor syscalls
void graphics_draw_cursor_syscall();
void graphics_erase_cursor_syscall();
void graphics_store_char_syscall(char c, uint8_t color);
void graphics_set_cursor_colors_syscall(uint8_t fg_color, uint8_t bg_color);

// Command processing
void process_command();

// Enhanced tab completion functions
void handle_tab_completion();
completion_context analyze_completion_context();
void find_command_completions(const char* prefix);
void find_flag_completions(const char* command, const char* prefix);
void find_file_completions(const char* command, const char* prefix);
void find_directory_completions(const char* prefix);
void find_path_completions(const char* path_input);
void find_pipeline_completions(const char* prefix);
void find_smart_file_completions(const char* command, const char* prefix);
void apply_current_completion();
void reset_completion_state();
bool starts_with(const char* str, const char* prefix);

typedef enum {
    ARG_NONE,           // No arguments needed
    ARG_TEXT,           // Free text/string (no completion)
    ARG_INTEGER,        // Integer value (no completion)
    ARG_NEW_NAME,       // New file/directory name (no completion)
    ARG_EXISTING_FILE,  // Existing file (file completion)
    ARG_EXISTING_DIR,   // Existing directory (directory completion)
    ARG_EXISTING_ANY,   // Existing file or directory
    ARG_MIXED           // Special handling needed
} arg_type_t;

arg_type_t get_command_arg_type(const char* command, int arg_position);
int get_current_arg_position(const char* command);

// Path utility functions
bool is_absolute_path(const char* path);
bool is_relative_path(const char* path);
void normalize_path(const char* input_path, char* output_path);
void split_path(const char* path, char* directory, char* filename);
uint32_t resolve_path_inode(const char* path);
bool flag_already_exists(const char* flag);

// String utility functions
char* strchr_local(const char* str, int c);
void strcat_local(char* dest, const char* src);

// External function from cd.c
extern void cd_root();

#endif