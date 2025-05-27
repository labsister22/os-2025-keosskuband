#include "header/usermode/commands/ls.h"

void ls(char* str) {
    uint8_t dir_data[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_data,
        .name = DIR_INFO.dir[DIR_INFO.current_dir].dir_name,
        .name_len = strlen(DIR_INFO.dir[DIR_INFO.current_dir].dir_name),
        .parent_inode = DIR_INFO.current_dir == 0 ? 1 : DIR_INFO.dir[DIR_INFO.current_dir - 1].inode,
        .buffer_size = BLOCK_SIZE,
        .is_directory = true
    };
    int32_t retcode = 0;
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0) {
        if (retcode == 4) {
            print_string_colored("Not a directory\n", COLOR_RED);
        } else {
            print_string_colored("Error reading directory\n", COLOR_RED);
        }

        return;
    }

    struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        if (entry->file_type == EXT2_FT_DIR) {
            print_string_colored(entry->name, COLOR_GREEN);
        } else if (entry->file_type == EXT2_FT_REG_FILE) {
            print_string_colored(entry->name, COLOR_WHITE);
        } else {
            print_string_colored(entry->name, COLOR_LIGHT_GRAY);
        }
        cursor.row++;
        cursor.col = 0;

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }
}