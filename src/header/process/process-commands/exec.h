#ifndef EXEC_KERNEL_COMMAND
#define EXEC_KERNEL_COMMAND

#include "header/process/process.h"

void exec(char* exec_filename, uint32_t parent_inode, int32_t* retcode);

#endif 
