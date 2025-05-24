#include "header/usermode/commands/mkdir.h"

void mkdir(char* str) {
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