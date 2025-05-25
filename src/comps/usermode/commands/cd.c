#include "header/usermode/commands/cd.h"

void cd(char* str) {
    absolute_dir_info bef = DIR_INFO;
    char parsed_dir[20][12] = {0};
    int parsed_count = 0;

    int j = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (parsed_count == 20) {
            print_string_colored("Too many directories specified\n", COLOR_RED);
            print_newline();

            return;
        }

        if (str[i] == '/') {
            if (j > 0) {
                parsed_dir[parsed_count][j] = '\0';
                parsed_count++;
                j = 0;
            }
        } else {
            if (j < 11) { // Ensure we don't overflow the buffer
                parsed_dir[parsed_count][j] = str[i];
                j++;
            }
            else {
                print_string_colored("Directory name too long\n", COLOR_RED);
                print_newline();

                return;
            }
        }
    }
    parsed_count++;

    for (int step = 0; step < parsed_count; step++) {
        char* next_dir = parsed_dir[step];

        if (strcmp(next_dir, ".") == 0) {
            continue; // Skip current directory
        }

        if (strcmp(next_dir, "..") == 0) {
            // Handle parent directory
            if (bef.current_dir > 0) {
                bef.current_dir--;
                continue;
            }
        }

        if (strcmp(next_dir, "") == 0) {
            continue; // Skip empty segments
        }

        if (bef.current_dir >= 50) {
            print_string_colored("Maximum directory depth reached\n", COLOR_RED);
            print_newline();

            return;
        }

        uint8_t dir_data[BLOCK_SIZE];
        struct EXT2DriverRequest request = {
            .buf = dir_data,
            .name = bef.dir[bef.current_dir].dir_name,
            .name_len = strlen(bef.dir[bef.current_dir].dir_name),
            .parent_inode = bef.current_dir == 0 ? 1 : bef.dir[bef.current_dir - 1].inode,
            .buffer_size = BLOCK_SIZE,
            .is_directory = true
        };

        int32_t retcode = 0;
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);

        if (retcode != 0) {
            print_string_colored("Error accessing directory: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();
            
            return;
        }

        struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;
        bool found = false;

        uint32_t offset = 0;
        while (offset < BLOCK_SIZE) {
            if (entry->inode != 0 && entry->name_len == strlen(next_dir) &&
                memcmp(entry->name, next_dir, strlen(next_dir)) == 0) {
                found = true;
                break;
            }

            offset += entry->rec_len;
            if (offset < BLOCK_SIZE) {
                entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
            }
        }

        if (!found) {
            print_string_colored("Directory not found: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();

            return;
        }

        if (entry->file_type != EXT2_FT_DIR) {
            print_string_colored("Not a directory: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();

            return;
        }

        // Update current directory
        bef.current_dir++;
        bef.dir[bef.current_dir].inode = entry->inode;
        memcpy(bef.dir[bef.current_dir].dir_name, entry->name, entry->name_len);
        bef.dir[bef.current_dir].dir_name[entry->name_len] = '\0';
        bef.dir[bef.current_dir].dir_name_len = entry->name_len;
    }

    DIR_INFO = bef;
}

void cd_root() {
    DIR_INFO.current_dir = 0;
    DIR_INFO.dir[0].inode = 1;
    memcpy(DIR_INFO.dir[0].dir_name, ".", 1);
    DIR_INFO.dir[0].dir_name[1] = '\0'; // Null-terminate the string
    DIR_INFO.dir[0].dir_name_len = 1;
} 