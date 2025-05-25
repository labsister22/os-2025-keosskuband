#ifndef PS_USER_COMMAND
#define PS_USER_COMMAND

#include "header/usermode/user-shell.h"

typedef struct {
    int32_t pid;
    char name[10];
    int8_t state;
} ProcessMetadata;

void ps();

#endif 
