#include <stdint.h>
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"

#define BLOCK_COUNT 16
#define FRAMEBUFFER_WIDTH  80
#define FRAMEBUFFER_HEIGHT 25

typedef struct {
    int32_t row;
    int32_t col;
} CP;

CP cursor = {0, 0};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int main(void) {
    struct BlockBuffer      bl[3]   = {0};
    struct EXT2DriverRequest request = {
        .buf                   = &bl,
        .name                  = "shell",
        .parent_inode          = 1,
        .buffer_size           = BLOCK_SIZE * BLOCK_COUNT,
        .name_len = 5,
    };

    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    // if (retcode == 0)
    syscall(6, (uint32_t) "owo\n", 4, (uint32_t) &cursor);
    cursor.row++;

    char buf;
    syscall(7, 0, 0, 0);
    while (true) {
        syscall(4, (uint32_t) &buf, 0, 0);

        if (buf != 0x0) {
            syscall(5, (uint32_t) &buf, 0xF, (uint32_t) &cursor);
            cursor.col++;
            if (cursor.col >= FRAMEBUFFER_WIDTH) {
                cursor.col = 0;
                cursor.row++;
            }
        }
    }

    // activate_keyboard_interrupt();
    // framebuffer_set_cursor(15, 0);
    // int row = 15, col = 0;
    // keyboard_state_activate();
    // while (true) {
    //     char c;
    //     get_keyboard_buffer(&c);

    //     // Handle keyboard input
    //     if (c != 0) {
    //         framebuffer_write(row, col++, c, 0xE, 0);
    //         if (col >= FRAMEBUFFER_WIDTH) {
    //             col = 0;
    //             row++;
    //             if (row >= FRAMEBUFFER_HEIGHT) {
    //                 framebuffer_clear();
    //                 print_string("Paging tests completed. Press keys to see them echo here:", 0, 0, 0xF);
    //                 row = 2;
    //                 col = 0;
    //             }
    //         }
    //     }
    // }

    return 0;
}
