#include "header/usermode/commands/cd.h"

void cd(char* str) {
    absolute_dir_info bef = DIR_INFO;
    char parsed_dir[20][12] = {0};
    int parsed_count = 0;

    int j = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (parsed_count == 20) {
            syscall(6, (uint32_t)"Too many directories in path\n", 29, (uint32_t)&cursor);
            cursor.row++;
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
                syscall(6, (uint32_t)"Directory name too long\n", 24, (uint32_t)&cursor);
                cursor.row++;
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
            syscall(6, (uint32_t)"Maximum directory limit reached\n", 33, (uint32_t)&cursor);
            cursor.row++;
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
            // error
            syscall(6, (uint32_t)"Error accessing directory\n", 26, (uint32_t)&cursor);
            cursor.row++;
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
            syscall(6, (uint32_t)"Directory not found\n", 20, (uint32_t)&cursor);
            cursor.row++;
            return;
        }

        if (entry->file_type != EXT2_FT_DIR) {
            syscall(6, (uint32_t)"Not a directory\n", 16, (uint32_t)&cursor);
            cursor.row++;
            return;
        }

        // Update current directory
        bef.current_dir++;
        bef.dir[bef.current_dir].inode = entry->inode;
        memcpy(bef.dir[bef.current_dir].dir_name, entry->name, entry->name_len);
        bef.dir[bef.current_dir].dir_name[entry->name_len] = '\0';
        bef.dir[bef.current_dir].dir_name_len = entry->name_len;
    }

    DIR_INFO = bef; // Update the global directory info

    // if (strlen(str) > 12) {
    //     syscall(6, (uint32_t)"Directory name too long\n", 24, (uint32_t)&cursor);
    //     cursor.row++;
    //     return;
    // }

    // if (!memcmp(str, ".", 1) && strlen(str) == 1) {
    //     return;
    // }

    // uint8_t dir_data[BLOCK_SIZE];
    // struct EXT2DriverRequest request = {
    //     .buf = dir_data,
    //     .name = DIR_INFO.dir[DIR_INFO.current_dir].dir_name,
    //     .name_len = strlen(DIR_INFO.dir[DIR_INFO.current_dir].dir_name),
    //     .parent_inode = DIR_INFO.current_dir == 0 ? 1 : DIR_INFO.dir[DIR_INFO.current_dir - 1].inode,
    //     .buffer_size = BLOCK_SIZE,
    //     .is_directory = true
    // };
    // int32_t retcode = 0;
    // syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    // if (retcode != 0) {
    //     // error
    //     return;
    // }

    // struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    // bool found = false;
    // uint32_t offset = 0;
    // while (offset < BLOCK_SIZE) {
    //     // syscall(6, entry->name, entry->name_len, (uint32_t)&cursor);
    //     // cursor.row++;

    //     if (entry->inode != 0 && entry->name_len == strlen(str) &&
    //         memcmp(entry->name, str, strlen(str)) == 0) {
    //         found = true;
    //         break;
    //     }

    //     offset += entry->rec_len;
    //     if (offset < BLOCK_SIZE) {
    //         entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
    //     }
    // }

    // if (!found) {
    //     syscall(6, (uint32_t)"Directory not found\n", 20, (uint32_t)&cursor);
    //     cursor.row++;
    //     return;
    // }

    // if (entry->file_type != EXT2_FT_DIR) {
    //     syscall(6, (uint32_t)"Not a directory\n", 16, (uint32_t)&cursor);
    //     cursor.row++;
    //     return;
    // }

    // // handle "cd .."
    // if (!memcmp(str, "..", 2)) {
    //     // update current directory to grandparent
    //     DIR_INFO.current_dir = DIR_INFO.current_dir == 0 ? 0 : DIR_INFO.current_dir - 1;
    // }
    // else {
    //     // update current directory
    //     DIR_INFO.current_dir++;
    //     DIR_INFO.dir[DIR_INFO.current_dir].inode = entry->inode;
    //     memcpy(DIR_INFO.dir[DIR_INFO.current_dir].dir_name, entry->name, entry->name_len);
    //     DIR_INFO.dir[DIR_INFO.current_dir].dir_name[entry->name_len] = '\0';
    //     DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len = entry->name_len;
    // }
}

void cd_root() {
    DIR_INFO.current_dir = 0;
    DIR_INFO.dir[0].inode = 1;
    memcpy(DIR_INFO.dir[0].dir_name, ".", 1);
    DIR_INFO.dir[0].dir_name[1] = '\0'; // Null-terminate the string
    DIR_INFO.dir[0].dir_name_len = 1;
} 