#include "header/usermode/user-shell.h"
#include "header/usermode/commands/mv.h"
#include "header/stdlib/string.h"
#include "header/stdlib/strops.h"

void mv(char *src, char* dest, char* dump) {
  if (dump != NULL && *dump != '\0') {
    print_string_colored("Too many arguments\n", COLOR_RED);
    print_newline();

    print_string_colored("Usage: mv <src> <dest>", COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (src == NULL || *src == '\0' || dest == NULL || *dest == '\0') {
    print_string_colored("Usage: mv <src> <dest>", COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (strlen(src) > 12 || strlen(dest) > 12) {
    print_string_colored("File name too long\n", COLOR_RED);
    print_newline();
    return;
  }

  if (!memcmp(src, ".", 1) && strlen(src) == 1) {
    print_string_colored("Cannot move file with name '.'\n", COLOR_RED);
    print_newline();
    return;
  }

  if (!memcmp(dest, ".", 1) && strlen(dest) == 1) {
    print_string_colored("Cannot move file to name '.'\n", COLOR_RED);
    print_newline();
    return;
  }

  char buf[5120];
  memset(buf, 0, sizeof(buf));

  struct EXT2DriverRequest request = {
    .name = src,
    .name_len = strlen(src),
    .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
    .buf = buf,
    .buffer_size = sizeof(buf),
    .is_directory = false
  };

  int retcode = 0;
  syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);

  if (retcode == 1) {
    print_string_colored("Source file is a directory\n", COLOR_RED);
    print_newline();
    return;
  }
  else if (retcode == 2) {
    print_string_colored("Cannot move : file too large\n", COLOR_RED);
    print_newline();
    return;
  }
  else if (retcode == 3) {
    print_string_colored("Source file not found\n", COLOR_RED);
    print_newline();
    return;
  }
  else if (retcode != 0) {
    print_string_colored("Error reading source file\n", COLOR_RED);
    print_newline();
    return;
  }

  if (dest[0] == '.') {
    // case : move

    // move to the destination directory
    absolute_dir_info bef = DIR_INFO;

    char parsed_dir[20][12] = {0};
    int parsed_count = 0;

    int j = 0;
    for (int i = 0; i < strlen(dest); i++) {
        if (parsed_count == 20) {
            print_string_colored("Too many directories specified\n", COLOR_RED);
            print_newline();

            return;
        }

        if (dest[i] == '/') {
            if (j > 0) {
                parsed_dir[parsed_count][j] = '\0';
                parsed_count++;
                j = 0;
            }
        } else {
            if (j < 11) { // Ensure we don't overflow the buffer
                parsed_dir[parsed_count][j] = dest[i];
                j++;
            }
            else {
                print_string_colored("Directory name too long\n", COLOR_RED);
                print_newline();

                return;
            }
        }
    }
    parsed_count++;

    for (int step = 0; step < parsed_count; step++) {
        char* next_dir = parsed_dir[step];

        if (strcmp(next_dir, ".") == 0) {
            continue; // Skip current directory
        }

        if (strcmp(next_dir, "..") == 0) {
            // Handle parent directory
            if (bef.current_dir > 0) {
                bef.current_dir--;
                continue;
            }
        }

        if (strcmp(next_dir, "") == 0) {
            continue; // Skip empty segments
        }

        if (bef.current_dir >= 50) {
            print_string_colored("Maximum directory depth reached\n", COLOR_RED);
            print_newline();

            return;
        }

        uint8_t dir_data[BLOCK_SIZE];
        struct EXT2DriverRequest request = {
            .buf = dir_data,
            .name = bef.dir[bef.current_dir].dir_name,
            .name_len = strlen(bef.dir[bef.current_dir].dir_name),
            .parent_inode = bef.current_dir == 0 ? 1 : bef.dir[bef.current_dir - 1].inode,
            .buffer_size = BLOCK_SIZE,
            .is_directory = true
        };

        int32_t retcode = 0;
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);

        if (retcode != 0) {
            print_string_colored("Error accessing directory: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();
            
            return;
        }

        struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;
        bool found = false;

        uint32_t offset = 0;
        while (offset < BLOCK_SIZE) {
            if (entry->inode != 0 && entry->name_len == strlen(next_dir) &&
                memcmp(entry->name, next_dir, strlen(next_dir)) == 0 && 
                entry->name_len == strlen(next_dir)) {
                found = true;
                break;
            }

            offset += entry->rec_len;
            if (offset < BLOCK_SIZE) {
                entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
            }
        }

        if (!found) {
            print_string_colored("Directory not found: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();

            return;
        }

        if (entry->file_type != EXT2_FT_DIR) {
            print_string_colored("Not a directory: ", COLOR_RED);
            print_string_colored(next_dir, COLOR_RED);
            print_newline();

            return;
        }

        // Update current directory
        bef.current_dir++;
        bef.dir[bef.current_dir].inode = entry->inode;
        memcpy(bef.dir[bef.current_dir].dir_name, entry->name, entry->name_len);
        bef.dir[bef.current_dir].dir_name[entry->name_len] = '\0';
        bef.dir[bef.current_dir].dir_name_len = entry->name_len;
    }

    // after moving to the destination directory, write the file
    request.name = src;
    request.name_len = strlen(src);
    request.buffer_size = sizeof(buf);
    request.parent_inode = bef.dir[bef.current_dir].inode;
    request.is_directory = false;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 0) {
      print_string_colored("Moved : ", COLOR_LIGHT_BLUE);
      print_string_colored(src, COLOR_GREEN);
      print_string_colored(" to ", COLOR_LIGHT_BLUE);
      print_string_colored(dest, COLOR_GREEN);
      print_newline();

      // delete request
      struct EXT2DriverRequest delete_request = {
        .name = src,
        .name_len = strlen(src),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = buf,
        .buffer_size = sizeof(buf),
        .is_directory = false
      };
      int inode = 0;
      syscall(53, (uint32_t)&delete_request, (uint32_t)&inode, 0);
      dynamic_array_idx_free(inode);

      int delete_retcode = 0;
      syscall(3, (uint32_t)&delete_request, (uint32_t)&delete_retcode, 0);

      // add to indexing
      struct EXT2DriverRequest inode_request = {
        .name = dest,
        .name_len = strlen(dest),
        .parent_inode = bef.dir[bef.current_dir].inode,
        .buf = NULL,
        .buffer_size = 0,
        .is_directory = false
      };
      int new_inode = 0;
      syscall(53, (uint32_t)&inode_request, (uint32_t)&new_inode, 0);
      dynamic_array_add(dest, new_inode, bef.dir[bef.current_dir].inode);
    } else {
      if (retcode == 1) {
        print_string_colored("Destination file already exists\n", COLOR_RED);
      } else if (retcode == 2) {
        print_string_colored("Parent is not a directory\n", COLOR_RED);
      }
      print_newline();
    }
  } else {
    // case rename

    // write again
    request.name = dest;
    request.name[strlen(dest)] = '\0'; // Ensure null-termination
    request.name_len = strlen(dest);
    request.buffer_size = sizeof(buf);
    request.parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 0) {
      print_string_colored("Renamed : ", COLOR_LIGHT_BLUE);
      print_string_colored(src, COLOR_GREEN);
      print_string_colored(" to ", COLOR_LIGHT_BLUE);
      print_string_colored(dest, COLOR_GREEN);
      print_newline();

      // delete request
      struct EXT2DriverRequest delete_request = {
        .name = src,
        .name_len = strlen(src),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = buf,
        .buffer_size = sizeof(buf),
        .is_directory = false
      };
      int inode = 0;
      syscall(53, (uint32_t)&delete_request, (uint32_t)&inode, 0);
      dynamic_array_idx_free(inode);

      // delete first
      retcode = 0;
      syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

      // add to indexing
      struct EXT2DriverRequest inode_request = {
        .name = dest,
        .name_len = strlen(dest),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = NULL,
        .buffer_size = 0,
        .is_directory = false
      };
      int new_inode = 0;
      syscall(53, (uint32_t)&inode_request, (uint32_t)&new_inode, 0);
      dynamic_array_add(dest, new_inode, DIR_INFO.dir[DIR_INFO.current_dir].inode);
    } else {
      print_string_colored("Error renaming file\n", COLOR_RED);
      print_newline();
    }
  }
}