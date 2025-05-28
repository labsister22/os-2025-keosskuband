#include "header/usermode/commands/kill.h"
#include "header/stdlib/strops.h"

void kill(char* pid) {
    
    uint32_t pid_num;
    bool success;
    if (str_to_int(pid, &pid_num)) {
        if (pid_num == 0) {
            print_string_at_cursor("Cant kill this process. bleeeh");
            print_newline();
            return;
        }

        syscall(32, pid_num, (uint32_t) &success, 0);

        if (success) {
            print_string_at_cursor("Process killed successfully");
        }
        else {
            print_string_at_cursor("Failed to kill process.");
        }
    }
    else {
        print_string_at_cursor("Not a valid PID");
    }
    print_newline();
}