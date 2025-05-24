#include "header/usermode/commands/mkdir.h"

void mkdir(char* str) {
    if (strlen(str) > 12) {
        syscall(6, (uint32_t)"Directory name too long\n", 24, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
    if (!memcmp(str, ".", 1) && strlen(str) == 1) {
        syscall(6, (uint32_t)"Cannot create directory with name '.'\n", 38, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
    if (!memcmp(str, "..", 2) && strlen(str) == 2) {
        syscall(6, (uint32_t)"Cannot create directory with name '..'\n", 38, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
    if (strlen(str) == 0) {
        syscall(6, (uint32_t)"Directory name cannot be empty\n", 31, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
    if (DIR_INFO.current_dir >= 50) {
        syscall(6, (uint32_t)"Maximum directory limit reached\n", 33, (uint32_t)&cursor);
        cursor.row++;
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
        syscall(6, (uint32_t)"Directory already exists\n", 24, (uint32_t)&cursor);
        cursor.row++;
        return;
    } else if (retcode == 2) {
        syscall(6, (uint32_t)"Parent directory not found\n", 27, (uint32_t)&cursor);
        cursor.row++;
        return;
    } else if (retcode == 0) {
        syscall(6, (uint32_t)"Directory created successfully\n", 30, (uint32_t)&cursor);
        cursor.row++;
        return;
    } else {
        syscall(6, (uint32_t)"Unknown error\n", 14, (uint32_t)&cursor);
        cursor.row++;
        return;
    }
}