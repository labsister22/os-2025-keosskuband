#ifndef FIND_COMMAND
#define FIND_COMMAND

#include "header/usermode/user-shell.h"
#include "header/filesys/ext2.h"
#include "header/stdlib/string.h"
#include "header/stdlib/strops.h"

extern char find_path[2048];
extern int find_path_len;
extern int find_parent_inode;
extern int find_cur_inode;
extern char find_cur_directory[12];
extern char find_cur_directory_len;

void find(char* str, char* args1);
void find_recursive(char* str);
void initialize_indexing();
void indexing_recursive();
void debug_indexing();
void print_path(int idx);
void find_by_index(char* str);

#endif 
