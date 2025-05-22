#include "header/usermode/commands/help.h"

void help() {
    print_string_colored("Available commands:", COLOR_YELLOW);
    print_newline();
    print_string_colored("  help - Show this help message", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  clear - Clear the screen", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  echo [text] - Echo the text", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  edit - Test line editing features", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  debug - Debug mode to see key scancodes", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  exit - Exit the shell", COLOR_LIGHT_GRAY);
    print_newline();
    print_newline();
    print_string_colored("Editing Features:", COLOR_YELLOW);
    print_newline();
    print_string_colored("  <- -> Arrow Keys - Move cursor in line", COLOR_LIGHT_GREEN);
    print_newline();
    print_string_colored("  Ctrl+A/Ctrl+D - Alternative cursor movement", COLOR_LIGHT_GREEN);
    print_newline();
    print_string_colored("  Backspace - Delete char before cursor", COLOR_LIGHT_GREEN);
    print_newline();
    print_string_colored("  Insert anywhere - Type to insert text", COLOR_LIGHT_GREEN);
    print_newline();
}