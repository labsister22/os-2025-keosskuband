#ifndef EXEC_USER_COMMAND
#define EXEC_USER_COMMAND

#include "header/usermode/user-shell.h"

void exec(char* exec_filename, uint32_t parent_inode);

#endif 
