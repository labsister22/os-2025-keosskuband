#ifndef USER_SHELL
#define USER_SHELL

#include <stdint.h>
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

typedef struct {
    int32_t row;
    int32_t col;
} CP;


void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// output commands
void print_string_at_cursor(const char* str);
void print_string_colored(const char* str, uint8_t color);
void print_newline();
void set_hardware_cursor();

#endif