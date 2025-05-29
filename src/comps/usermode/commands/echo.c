#include "header/usermode/commands/echo.h"

void echo(char* str, int length) {
    str[length] = '\0'; // Ensure null-termination
    print_string_at_cursor(str);
    print_newline();
}