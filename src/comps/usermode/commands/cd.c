#include "header/usermode/commands/cd.h"

void cd(char* str) {
    if (strlen(str) > 12) {
        syscall(6, (uint32_t)"Directory name too long\n", 24, (uint32_t)&cursor);
        cursor.row++;
        return;
    }

    if (!memcmp(str, ".", 1) && strlen(str) == 1) {
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

    bool found = false;
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        // syscall(6, entry->name, entry->name_len, (uint32_t)&cursor);
        // cursor.row++;

        if (entry->inode != 0 && entry->name_len == strlen(str) &&
            memcmp(entry->name, str, strlen(str)) == 0) {
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

    // handle "cd .."
    if (!memcmp(str, "..", 2)) {
        // update current directory to grandparent
        DIR_INFO.current_dir = DIR_INFO.current_dir == 0 ? 0 : DIR_INFO.current_dir - 1;
    }
    else {
        // update current directory
        DIR_INFO.current_dir++;
        DIR_INFO.dir[DIR_INFO.current_dir].inode = entry->inode;
        memcpy(DIR_INFO.dir[DIR_INFO.current_dir].dir_name, entry->name, entry->name_len);
        DIR_INFO.dir[DIR_INFO.current_dir].dir_name[entry->name_len] = '\0';
        DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len = entry->name_len;
        
        // path.path[path.len++] = '/';
        // for (int i = 0; i < entry->name_len; i++) {
        //     path.path[path.len++] = entry->name[i];
        // }
        // path.path[path.len++] = '\0';
    }
}

void cd_root() {
    DIR_INFO.current_dir = 0;
    DIR_INFO.dir[0].inode = 1;
    memcpy(DIR_INFO.dir[0].dir_name, ".", 1);

    path.path[0] = '.';
    path.len = 1;
} 