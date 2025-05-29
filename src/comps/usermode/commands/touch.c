#include "header/usermode/commands/touch.h"

void touch(char* name, char* content, int len) {
    if (name == NULL || strlen(name) == 0) {
        print_string_colored("Invalid file name\n", COLOR_RED);
        print_newline();
        return;
    }
    if (strlen(name) > 12) {
        print_string_colored("File name too long\n", COLOR_RED);
        print_newline();
        return;
    }
    if (!memcmp(name, ".", 1) && strlen(name) == 1) {
        print_string_colored("Cannot create file with name '.'\n", COLOR_RED);
        print_newline();
        return;
    }
    if (!memcmp(name, "..", 2) && strlen(name) == 2) {
        print_string_colored("Cannot create file with name '..'\n", COLOR_RED);
        print_newline();
        return;
    }
    if (name[0] == '.' || name[0] == '/') {
        print_string_colored("File name cannot start with '.' or '/'\n", COLOR_RED);
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

    struct EXT2DriverRequest request = {
        .buf = content, // Content to write, can be NULL if no content
        .name = name,
        .name_len = strlen(name),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buffer_size = len, // Buffer size is the length of content
        .is_directory = false // Touch creates a file, not a directory
    };  
    int retcode = 0;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0); 

    if (retcode != 0) {
        print_string_colored("Failed to create file\n", COLOR_RED);
        print_newline();
        return;
    }

    print_string_colored("File created successfully\n", COLOR_GREEN);
    print_newline();

    // Add to indexing
    struct EXT2DriverRequest inode_request = {
        .name = name,
        .name_len = strlen(name),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = NULL,
        .buffer_size = 0,
        .is_directory = false
    };
    int new_inode = 0;
    syscall(53, (uint32_t)&inode_request, (uint32_t)&new_inode, 0);
    dynamic_array_add(name, new_inode, DIR_INFO.dir[DIR_INFO.current_dir].inode);

    return;
}
