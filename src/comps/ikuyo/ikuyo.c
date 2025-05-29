#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/filesys/ext2.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : : "r"(eax));
    __asm__ volatile("int $0x30");
}

int main() {
  int FRAME_COUNT = 44;
  int FRAME_PER_SEGMENT = 4;

  int segment_count = FRAME_COUNT / FRAME_PER_SEGMENT;
  for (int counter = 0; counter < 10; counter++) {
    for (int i = 0; i < segment_count; i++) {
      char ikuyokita_frames[4][200*512];
      struct EXT2DriverRequest request;
      request.buf = (uint8_t*) &ikuyokita_frames;
      request.parent_inode = 2;
      request.buffer_size = FRAME_PER_SEGMENT * 200 * 512;
      request.is_directory = 0;

      // Set name and name_len for each segment
      if (i == 0) {
        request.name = "ikuyokita0-3";
        request.name_len = 12;
      } else if (i == 1) {
        request.name = "ikuyokita4-7";
        request.name_len = 12;
      } else if (i == 2) {
        request.name = "ikuyokita8-11";
        request.name_len = 13;
      } else if (i == 3) {
        request.name = "ikuyokita12-15";
        request.name_len = 14;
      } else if (i == 4) {
        request.name = "ikuyokita16-19";
        request.name_len = 14;
      } else if (i == 5) {
        request.name = "ikuyokita20-23";
        request.name_len = 14;
      } else if (i == 6) {
        request.name = "ikuyokita24-27";
        request.name_len = 14;
      } else if (i == 7) {
        request.name = "ikuyokita28-31";
        request.name_len = 14;
      } else if (i == 8) {
        request.name = "ikuyokita32-35";
        request.name_len = 14;
      } else if (i == 9) {
        request.name = "ikuyokita36-39";
        request.name_len = 14;
      } else if (i == 10) {
        request.name = "ikuyokita40-43";
        request.name_len = 14;
      }

      uint32_t res = 10;
      syscall(0, (uint32_t) &request, (uint32_t) &res, 0);

      for (int j = 0; j < FRAME_PER_SEGMENT; j++) {
        syscall(20, (uint32_t)ikuyokita_frames[j], 200, 512);
      }
    }
  }
}