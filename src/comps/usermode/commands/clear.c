#include "header/usermode/commands/clear.h"

void clear() {
    cursor.row = 0;
    cursor.col = 0;
    set_hardware_cursor();
    
    syscall(17, COLOR_BLACK, 0, 0);
    
    cursor.row = 0;
    cursor.col = 0;
}