#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/sleep.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : : "r"(eax));
    __asm__ volatile("int $0x30");
}

int main(void) {
    while (true){
        __asm__ volatile("hlt");
    }
    return 0;
}