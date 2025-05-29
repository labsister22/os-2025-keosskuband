#ifndef RM_H
#define RM_H

#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"

void rm(char *args1, char* args2, char* args3); 
void rm_recursive();

#endif 