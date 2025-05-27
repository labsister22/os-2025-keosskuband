#ifndef CP_H
#define CP_H

#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"

void cp(char *src, char* dest, char* flag, char* dump); 
void cp_recursive(int cur_dest_inode, int parent_dest_inode);

#endif 