#include "header/usermode/commands/cd.h"

void cd(char* str) {
    uint8_t dir_data[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_data,
        .name = cur_directory.dir_name,
        .name_len = strlen(cur_directory.dir_name),
        .parent_inode = cur_directory.parent_inode,
        .buffer_size = BLOCK_SIZE,
        .is_directory = true
    };
    int32_t retcode;
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0) {
        // error
        return;
    }

    struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    bool found = false;
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        syscall(6, entry->name, entry->name_len, (uint32_t)&cursor);
        cursor.row++;
        
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

    // update current directory
    cur_directory.parent_inode = cur_directory.inode;
    cur_directory.inode = entry->inode;
    memcpy(cur_directory.dir_name, entry->name, entry->name_len);
    
    path.path[path.len++] = '/';
    for (int i = 0; i < entry->name_len; i++) {
        path.path[path.len++] = entry->name[i];
    }
    path.path[path.len++] = '\0';

    // update path
    // if (!memcmp(str, "..", 2)) {
    //     //remove some last char
    //     path.len -= strlen(cur_directory.dir_name) + 1;
    // }
    // else {
    //     path.path[path.len++] = '/';
    //     for (int i = 0; i < entry->name_len; i++) {
    //         path.path[path.len++] = entry->name[i];
    //     }
    //     path.path[path.len++] = '\0';
    // }
}

void cd_root() {
    cur_directory.inode = 1;
    cur_directory.parent_inode = 1;
    cur_directory.dir_name = ".";

    path.len = 0;
} 