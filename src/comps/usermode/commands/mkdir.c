#include "header/usermode/commands/mkdir.h"

void mkdir(char* str) {
    if (strlen(str) > 12) {
        print_string_colored("Directory name too long\n", COLOR_RED);
        print_newline();

        return;
    }
    if (!memcmp(str, ".", 1) && strlen(str) == 1) {
        print_string_colored("Cannot create directory with name '.'\n", COLOR_RED);
        print_newline();

        return;
    }
    if (!memcmp(str, "..", 2) && strlen(str) == 2) {
        print_string_colored("Cannot create directory with name '..'\n", COLOR_RED);
        print_newline();

        return;
    }
    if (strlen(str) == 0) {
        print_string_colored("Directory name cannot be empty\n", COLOR_RED);
        print_newline();        

        return;
    }
    
    bool contains_invalid_chars = false;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == '/' || str[i] == '\\' || str[i] == ':' || str[i] == '*' ||
            str[i] == '?' || str[i] == '"' || str[i] == '<' || str[i] == '>' ||
            str[i] == '|' || str[i] < 32 || str[i] > 126) {
            contains_invalid_chars = true;
            break;
        }
    }

    if (contains_invalid_chars) {
        print_string_colored("Directory name contains invalid characters\n", COLOR_RED);
        print_newline();

        return;
    }

    if (DIR_INFO.current_dir >= 50) {
        print_string_colored("Maximum directory depth reached\n", COLOR_RED);
        print_newline();
        
        return;
    }

    struct EXT2DriverRequest request = {
        .name = str,
        .name_len = strlen(str),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .is_directory = true
    }; 

    int32_t retcode = 0;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 1) {
        print_string_colored("Directory already exists\n", COLOR_RED);
        print_newline();

        return;
    } else if (retcode == 2) {
        print_string_colored("Parent directory not found\n", COLOR_RED);
        print_newline();

        return;
    } else if (retcode == 0) {
        print_string_colored("Directory created successfully\n", COLOR_GREEN);
        print_newline();

        struct EXT2DriverRequest inode_request = {
            .name = str,
            .name_len = strlen(str),
            .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
            .buf = NULL,
            .buffer_size = 0,
            .is_directory = true
        };
        int new_inode = 0;
        syscall(53, (uint32_t)&inode_request, (uint32_t)&new_inode, 0);
        dynamic_array_add(str, new_inode, DIR_INFO.dir[DIR_INFO.current_dir].inode);

        return;
    } else {
        print_string_colored("Unknown error occurred while creating directory\n", COLOR_RED);
        print_newline();

        return;
    }
}