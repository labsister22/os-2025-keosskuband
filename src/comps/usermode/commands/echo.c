#include "header/usermode/commands/echo.h"

void echo(char* str) {
    print_string_at_cursor(str);
    print_newline();
}