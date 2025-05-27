#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

extern void user_syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void sleep(int sleep_time);

#endif