// this is to be used by external executable files
// it needs a syscall
// but im assuming that any external executable files almost
// certainly already have syscall 

#include "header/stdlib/sleep.h"

void sleep(int sleep_time) {
    syscall(35, (uint32_t) &sleep_time, 0, 0);
}

lluint getCurrentTick() {
    lluint tick;
    syscall(36, (uint32_t) &tick, 0, 0);
    return tick;
}

