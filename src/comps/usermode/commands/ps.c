#include "header/usermode/commands/ps.h"
#include "header/stdlib/strops.h"

void ps() {
    uint32_t max_process;
    ProcessMetadata info;
    char pid[8];

    syscall(31, (uint32_t) &max_process, 0, 0);
    
    print_string_at_cursor("PID  -  Name  -  State");
    print_newline();

    for (uint32_t i = 0; i < max_process; i++) {
        syscall(30, (uint32_t) &info, i, 0);
        
        if (info.pid == -1) continue;

        int_toString(info.pid, pid);
        print_string_at_cursor(pid);

        print_string_at_cursor(" - ");
        print_string_at_cursor(info.name);

        switch (info.state) {
        case 0: // READY
            print_string_at_cursor(" - READY");
            break;
        case 1: // RUNNING
            print_string_at_cursor(" - RUNNING");
            break;
        case 2: // TERMINATED
            print_string_at_cursor(" - TERMINATED");
            break;
        default: // bruhh
            print_string_at_cursor(" - LAH KOK BISA");
            break;
        }

        print_newline();
    }
}