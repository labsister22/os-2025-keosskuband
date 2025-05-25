#ifndef _USER_SHELL
#define _USER_SHELL

#include <stdint.h>
#include <stdbool.h>
#include "header/filesys/ext2.h"

#define BLOCK_COUNT 16

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

// Additional constants and structures
#define MAX_INPUT_LENGTH 2048
#define MAX_ARGS_AMOUNT 10
#define MAX_ARGS_LENGTH 32
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

extern CP cursor;
extern str_path path;
extern absolute_dir_info DIR_INFO;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// output commands
void print_string_at_cursor(const char* str);
void print_string_colored(const char* str, uint8_t color);
void print_newline();
void set_hardware_cursor();

// history
void add_to_history(const char* command);
void load_history_entry(int index);
void handle_up_arrow();
void handle_down_arrow();

// misc
void update_cursor_row_col();
void print_prompt();


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

// Additional function declarations
void clear_input_buffer();
void redraw_input_line();
void show_cursor();
void hide_cursor();

#endif