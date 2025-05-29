#include "header/usermode/commands/help.h"

void help() {
    print_string_colored("Available commands:", COLOR_YELLOW);
    print_newline();
    print_string_colored("  help - Show this help message", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  clear - Clear the screen", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  ls - List files in current directory", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  cd <directory> - Move to spesified directory", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  mkdir <name> - Create a new directory", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  rm <filename> [--rf] - Delete a file.", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  cp <src> <dest> [--rf] - Copy a file to the target location", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  mv <src> <dest> [--rf] - move a file to the target location", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  find <filename> - find filename location", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  cat <filename> - Print out the specified file content", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  echo <text> - Echo the text", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  ps - List active process", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  exec <executables> - Run an executable file", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  kill <pid> - Kill an active process", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  sleep <time> - sleep current running process for the given amount time (ms)", COLOR_LIGHT_GRAY);
    print_newline();
    print_string_colored("  exit - Exit the shell", COLOR_LIGHT_GRAY);
    print_newline();
    print_newline();
}