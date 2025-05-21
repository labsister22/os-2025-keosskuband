#include <stdint.h>
#include "header/filesys/ext2.h"

#define BLOCK_COUNT 16

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
