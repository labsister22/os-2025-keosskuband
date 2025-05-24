#include "header/usermode/commands/ls.h"

void ls(char* str) {
    if (strlen(str) > 12) {
        syscall(6, (uint32_t)"Directory name too long\n", 24, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
    
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
        // error
        return;
    }

    struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        char buffer[20];
        memcpy(buffer, entry->name, entry->name_len);
        if (entry->file_type == EXT2_FT_DIR) {
            buffer[entry->name_len + 0] = '-';
            buffer[entry->name_len + 1] = 'D';
            buffer[entry->name_len + 2] = 'I';
            buffer[entry->name_len + 3] = 'R';
            buffer[entry->name_len + 4] = '\0';
            syscall(6, (uint32_t)buffer, entry->name_len + 4, (uint32_t)&cursor);
            cursor.row++;
        } else if (entry->file_type == EXT2_FT_REG_FILE) {
            buffer[entry->name_len + 0] = '-';
            buffer[entry->name_len + 1] = 'F';
            buffer[entry->name_len + 2] = 'I';
            buffer[entry->name_len + 3] = 'L';
            buffer[entry->name_len + 4] = '\0';
            syscall(6, (uint32_t)buffer, entry->name_len + 4, (uint32_t)&cursor);
            cursor.row++;
        } else if (entry->file_type == EXT2_FT_UNKNOWN) {
            buffer[entry->name_len + 0] = '-';
            buffer[entry->name_len + 1] = 'U';
            buffer[entry->name_len + 2] = 'N';
            buffer[entry->name_len + 3] = 'K';
            buffer[entry->name_len + 4] = '\0';
            syscall(6, (uint32_t)buffer, entry->name_len + 4, (uint32_t)&cursor);
            cursor.row++;
        }

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }
}