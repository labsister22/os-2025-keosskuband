#include "header/process/process-commands/exec.h"
#include "header/process/process.h"
#include "header/stdlib/strops.h"

void exec(char* exec_filename, uint32_t parent_inode, int32_t* retcode) {
    
    struct EXT2DriverRequest file = {
        .buf = (void*)0, // Start of user space
        .name = exec_filename,
        .parent_inode = parent_inode,
        .buffer_size = 0, // BROOOO HATI HATI FOR NOW GW HARDCODE BUAT CLOCK PLIS JANGAN DIGUNAKAN UNTUK APAPUN PLIS PLIS PLIS PLIS
        .name_len = strlen(exec_filename),
        .is_directory = false
    };
    
    *retcode = process_create_user_process(file);
}