#include "header/usermode/commands/clear.h"

extern CP cursor;

void clear() {
    cursor.row = 0;
    cursor.col = 0;
    set_hardware_cursor();
    
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 80; j++) {
            CP temp = {i, j};
            char space = ' ';
            syscall(5, (uint32_t)&space, 0x07, (uint32_t)&temp);
        }
    }
    cursor.row = 0;
    cursor.col = 0;
}