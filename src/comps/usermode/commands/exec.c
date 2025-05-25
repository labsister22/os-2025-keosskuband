#include "header/usermode/commands/exec.h"
#include "header/stdlib/strops.h"

void exec(char* exec_filename, uint32_t parent_inode) {
    int32_t retcode;


    // validate here plis
    if (parent_inode < 1 || strlen(exec_filename) < 1) {
        print_string_at_cursor("file doesnt exist");
        print_newline();
        return;
    }

    syscall(33, (uint32_t) exec_filename, parent_inode, (uint32_t) &retcode);

    switch (retcode) {
    case 0:
        print_string_at_cursor("New process created successfully");
        break;
    case 1:
        print_string_at_cursor("No free process");
        break;
    case 2:
        print_string_at_cursor("Invalid file. file is not a user file"); // keknya gitu deh mangsudnya
        break;
    case 3:
        print_string_at_cursor("Not enough memory lol. poor");
        break;
    case 4:
        print_string_at_cursor("Failed to read file");
        break;
    default:
        print_string_at_cursor("idk man something went wrong. like really wrong");
        break;
    }

    print_newline();
}