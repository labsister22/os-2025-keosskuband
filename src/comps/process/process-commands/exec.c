#include "header/process/process-commands/exec.h"
#include "header/process/process.h"
#include "header/stdlib/strops.h"

void exec(char* exec_filename, uint32_t parent_inode, int32_t* retcode) {
    
    struct EXT2DriverRequest file = {
        .buf = (void*)0, // Start of user space
        .name = exec_filename,
        .parent_inode = parent_inode,
        .buffer_size = 0x400000, // 4MB (1 page frame) - this should match the actual allocation
        .name_len = strlen(exec_filename),
        .is_directory = false
    };
    
    *retcode = process_create_user_process(file);
}