#include "comps/usermode/commands/apple.h"

void apple(CP* cursor){
  int FRAME_COUNT = 1095*2;
  char apple_frames[FRAME_COUNT/2][512];
  struct EXT2DriverRequest request = {
    .buf = (uint8_t*) &apple_frames,
    .name = "apple.txt",
    .parent_inode = 1,
    .buffer_size = FRAME_COUNT / 2 * 512,
    .name_len = 9,
    .is_directory = false,
  };
  uint32_t res = 10;
  syscall(0, (uint32_t) &request, &res, 0);

  char buffer = ' '; 
  for (int i = 10; i < FRAME_COUNT / 2; i++) {
    // first frame
    cursor->row = 0;
    cursor->col = 0;
    for (int j = 0; j < 2000; j++) {
      uint8_t bg_color = (apple_frames[i][j / 8]) & (1 << (7 - j % 8)) ? 0xF : 0x00;
      syscall(9, (uint32_t)&buffer, (uint32_t)&(PrintRequest){.size = 1, .font_color = 0xF, .bg_color = bg_color}, (uint32_t)cursor);

      cursor->col++;
      if (cursor->col >= 80) {
        cursor->col = 0;
        cursor->row++;
        if (cursor->row >= 25) {
          cursor->row = 0;
        }
      }
    }

    // second frame
    cursor->row = 0;
    cursor->col = 0;
    for (int j = 0; j < 2000; j++) {
      uint8_t bg_color = (apple_frames[i][256 + j / 8]) & (1 << (7 - (256 + j) % 8)) ? 0xF : 0x00;
      syscall(9, (uint32_t)&buffer, (uint32_t)&(PrintRequest){.size = 1, .font_color = 0xF, .bg_color = bg_color}, (uint32_t)cursor);

      cursor->col++;
      if (cursor->col >= 80) {
        cursor->col = 0;
        cursor->row++;
        if (cursor->row >= 25) {
          cursor->row = 0;
        }
      }
    }
  }
}