#ifndef _SLEEP_H
#define _SLEEP_H

#include <stdint.h>
#include <stddef.h>
typedef unsigned long long int lluint;

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void sleep(int sleep_time);
lluint getCurrentTick();

#endif