#include "header/stdlib/strops.h"
#include "header/usermode/commands/cat.h"

// cat only be able to open 10 * 512 characters

void cat(char* name) {
    if (name == NULL) {
        print_string_colored("Invalid arguments\n", COLOR_RED);
        print_newline();
        return;
    }

    if (strlen(name) > 12) {
        print_string_colored("File name too long\n", COLOR_RED);
        print_newline();
        return;
    }

    if (!memcmp(name, ".", 1) && strlen(name) == 1) {
        print_string_colored("Cannot open file with name '.'\n", COLOR_RED);
        print_newline();
        return;
    }

    if (!memcmp(name, "..", 2) && strlen(name) == 2) {
        print_string_colored("Cannot open file with name '..'\n", COLOR_RED);
        print_newline();
        return;
    }

    if (strlen(name) == 0) {
        print_string_colored("File name cannot be empty\n", COLOR_RED);
        print_newline();
        return;
    }

    bool contains_invalid_chars = false;
    for (int i = 0; i < strlen(name); i++) {
        if (name[i] == '/' || name[i] == '\\' || name[i] == ':' || name[i] == '*' ||
            name[i] == '?' || name[i] == '"' || name[i] == '<' || name[i] == '>' ||
            name[i] == '|' || name[i] < 32 || name[i] > 126) {
            contains_invalid_chars = true;
            break;
        }
    }

    if (contains_invalid_chars) {
        print_string_colored("File name contains invalid characters\n", COLOR_RED);
        print_newline();
        return;
    }

    char buffer[5120] = {0}; // 10 * 512 bytes
    struct EXT2DriverRequest request = {
        .name = name,
        .name_len = strlen(name),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = buffer,
        .buffer_size = 5120,
        .is_directory = false
    }; 

    int32_t retcode = 0;
    syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 1) {
        print_string_colored("File is not a regular file: ", COLOR_RED);
        print_string_colored(name, COLOR_RED);
        print_newline();
        return;
    }
    else if (retcode == 2) {
        print_string_colored("File is too large to read: ", COLOR_RED);
        print_string_colored(name, COLOR_RED);
        print_newline();
        return;
    }
    else if (retcode == 3) {
        print_string_colored("File is corrupted: ", COLOR_RED);
        print_string_colored(name, COLOR_RED);
        print_newline();
        return;
    }
    else if (retcode != 0) {
        print_string_colored("Unknown Error", COLOR_RED);
        print_newline();
    }

    print_string_colored("File content:\n", COLOR_GREEN);
    print_string_colored(buffer, COLOR_WHITE);
    print_newline();
}