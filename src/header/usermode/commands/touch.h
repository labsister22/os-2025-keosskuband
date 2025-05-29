#ifndef TOUCH_COMMAND
#define TOUCH_COMMAND

#include "header/usermode/user-shell.h"
#include "header/filesys/ext2.h"
#include "header/stdlib/string.h"
#include "header/stdlib/strops.h"

void touch(char* name, char* content, int len);

#endif 
