#ifndef PS_KERNEL_COMMAND
#define PS_KERNEL_COMMAND

#include "header/process/process.h"
 
typedef struct {
    int32_t pid;
    char name[10];
    int8_t state;
} ProcessMetadata;

void ps(ProcessMetadata* info, uint8_t idx);

#endif 
