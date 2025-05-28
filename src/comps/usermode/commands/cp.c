#include "header/stdlib/strops.h"
#include "header/usermode/commands/rm.h"
#include "header/usermode/commands/cd.h"
#include "header/usermode/commands/cp.h"
#include "header/usermode/commands/find.h"

void cp(char *src, char* dest, char* flag, char* dump) {
  bool recursive = false;

  if (dump != NULL && strlen(dump) > 0) {
    print_string_colored("Too many arguments\n", COLOR_RED);
    print_newline();

    print_string_colored("Usage: copy <src> <dest> [--rf]", COLOR_LIGHT_RED);
    print_newline();

    return;
  }

  if (flag != NULL && strcmp(flag, "--rf") == 0) {
    recursive = true;
  } else if (flag != NULL && strlen(flag) > 0) {
    print_string_colored("Invalid option: ", COLOR_RED);
    print_string_colored(flag, COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (dest == NULL || strlen(dest) == 0 || src == NULL || strlen(src) == 0) {
    print_string_colored("Usage: copy <src> <dest> [--rf]", COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (strlen(dest) > 12) {
    print_string_colored("File name too long\n", COLOR_RED);
    print_newline();
    return;
  }
  
  if ((!memcmp(dest, ".", 1) && strlen(dest) == 1) || (!memcmp(src, ".", 1) && strlen(src) == 1)) {
    print_string_colored("Cannot remove file with name '.'\n", COLOR_RED);
    print_newline();
    return;
  }

  if ((!memcmp(dest, "..", 2) && strlen(dest) == 2) || (!memcmp(src, "..", 2) && strlen(src) == 2)) {
    print_string_colored("Cannot remove file with name '..'\n", COLOR_RED);
    print_newline();
    return;
  }

  if (strlen(dest) == 0 || strlen(src) == 0) {
    print_string_colored("File name cannot be empty\n", COLOR_RED);
    print_newline();
    return;
  }

  if (!recursive) {
    char buffer[5120] = {0}; // 100 * 512 bytes
    struct EXT2DriverRequest request = {
      .name = src,
      .name_len = strlen(src),
      .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
      .buf = buffer,
      .buffer_size = 5120,
      .is_directory = false
    };
    int32_t retcode = 0;
    syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 1) {
      print_string_colored("File is not a regular file: ", COLOR_RED);
      print_string_colored(src, COLOR_RED);
    } else if (retcode == 2) {
      print_string_colored("File is too large to read: ", COLOR_RED);
      print_string_colored(src, COLOR_RED);
    } else if (retcode == 3) {
      print_string_colored("File is corrupted or not found: ", COLOR_RED);
      print_string_colored(src, COLOR_RED);
    } else if (retcode == -1) {
      print_string_colored("Unknown error occurred while reading source file\n", COLOR_RED);
    } else if (retcode == 0) {
      struct EXT2DriverRequest dest_request = {
        .name = dest,
        .name_len = strlen(dest),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .buf = buffer,
        .buffer_size = 5120,
        .is_directory = false
      };

      int32_t copy_retcode = 0;
      syscall(2, (uint32_t)&dest_request, (uint32_t)&copy_retcode, 0);
      if (copy_retcode == 0) {
        print_string_colored("created : ", COLOR_LIGHT_BLUE);
        print_string_colored(dest, COLOR_GREEN);
        print_newline();
      } else if (copy_retcode == 1) {
        print_string_colored("Destination file already exists\n", COLOR_RED);
      } else if (copy_retcode == 2) {
        print_string_colored("Invalid parent directory for destination\n", COLOR_RED);
      } else if (copy_retcode == -1) {
        print_string_colored("Unknown error occurred while copying file\n", COLOR_RED);
      } else {
        print_string_colored("Unknown error occurred\n", COLOR_RED);
      }
    }
    print_newline();
  }
  else if (recursive) {
    absolute_dir_info before = DIR_INFO;

    // Create destination directory
    struct EXT2DriverRequest request = {
        .name = dest,
        .name_len = strlen(dest),
        .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
        .is_directory = true
    }; 
    int32_t retcode = 0;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 1) {
        print_string_colored("Destination directory already exists\n", COLOR_RED);
        print_newline();
        return;
    } else if (retcode == 2) {
        print_string_colored("Invalid parent directory for destination\n", COLOR_RED);
        print_newline();
        return;
    } else if (retcode != 0) {
        print_string_colored("Unknown error occurred while creating destination directory\n", COLOR_RED);
        print_newline();
        return;
    }

    print_string_colored("created : ", COLOR_LIGHT_BLUE);
    print_string_colored(dest, COLOR_GREEN);
    print_newline();

    cd(dest);

    int dest_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;

    cd("..");
    cd(src);

    memcpy(find_path, dest, strlen(dest));
    find_path_len = strlen(dest);
    find_parent_inode = (DIR_INFO.current_dir == 0) ? 1 : DIR_INFO.dir[DIR_INFO.current_dir - 1].inode;
    find_cur_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
    memcpy(find_cur_directory, DIR_INFO.dir[DIR_INFO.current_dir].dir_name, DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len);
    find_cur_directory_len = DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len;

    cp_recursive(dest_inode);
  
    DIR_INFO = before;  // restore
  }
}

void cp_recursive(int cur_dest_inode) {
    if (find_path_len >= 2048) {
        return; // Path too long
    }

    uint8_t dir_data[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_data,
        .name = find_cur_directory,
        .name_len = find_cur_directory_len,
        .parent_inode = find_parent_inode,
        .buffer_size = BLOCK_SIZE,
        .is_directory = true
    };
    int32_t retcode = 0;
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0) {
        // error reading directory
        return;
    }

    struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    // First pass: recursively copy subdirectories
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 && 
            entry->file_type == EXT2_FT_DIR && 
            strcmp(entry->name, ".") != 0 && 
            strcmp(entry->name, "..") != 0) {

            // create new subdirectory
            struct EXT2DriverRequest copy_request = {
                .name = entry->name,
                .name_len = entry->name_len,
                .parent_inode = cur_dest_inode,
                .buf = NULL,
                .buffer_size = 0,
                .is_directory = true
            };

            int copy_retcode = 0;
            syscall(2, (uint32_t)&copy_request, (uint32_t)&copy_retcode, 0);

            if (copy_retcode == 0) {
                char cur_delete_path[2048];
                int cur_delete_path_len = find_path_len;
                memcpy(cur_delete_path, find_path, find_path_len);
                cur_delete_path[cur_delete_path_len++] = '/';
                for (int i = 0; i < entry->name_len; i++) {
                    cur_delete_path[cur_delete_path_len++] = entry->name[i];
                }
                cur_delete_path[cur_delete_path_len] = '\0';
                
                print_string_colored("created : ", COLOR_LIGHT_BLUE);
                print_string_colored(cur_delete_path, COLOR_GREEN);
                print_newline();

                // Recursively copy subdirectory contents
                find_path[find_path_len++] = '/';
                for (int i = 0; i < entry->name_len; i++) {
                    find_path[find_path_len++] = entry->name[i];
                }
                find_path[find_path_len] = '\0';

                // Save current state
                char bef_dir[12];
                int bef_dir_len = find_cur_directory_len;
                int bef_parent_inode = find_parent_inode;
                int bef_cur_inode = find_cur_inode;
                memcpy(bef_dir, find_cur_directory, find_cur_directory_len);

                // Update state for subdirectory
                memcpy(find_cur_directory, entry->name, entry->name_len);
                find_cur_directory_len = entry->name_len;
                find_parent_inode = find_cur_inode;
                find_cur_inode = entry->inode;

                char dir_data2[512] = {0};
                // find the next recursed inode
                struct EXT2DriverRequest read_request = {
                    .name = entry->name,
                    .name_len = entry->name_len,
                    .parent_inode = cur_dest_inode,
                    .buf = dir_data2,
                    .buffer_size = 512,
                    .is_directory = true
                };

                retcode = 0;
                syscall(1, (uint32_t)&read_request, (uint32_t)&retcode, 0);
                if (retcode != 0) {
                    print_string_colored("Error finding next destination inode\n", COLOR_RED);
                    return;
                }
                struct EXT2DirectoryEntry* entry2 = (struct EXT2DirectoryEntry*)dir_data2;
                cp_recursive(entry2->inode);

                // Restore state
                find_path_len -= entry2->name_len + 1;
                find_path[find_path_len] = '\0';
                memcpy(find_cur_directory, bef_dir, bef_dir_len);
                find_cur_directory_len = bef_dir_len;
                find_parent_inode = bef_parent_inode;
                find_cur_inode = bef_cur_inode;
            }
        }
        else if (entry->inode != 0 && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0 &&
                 entry->file_type == EXT2_FT_REG_FILE) {

            // read first
            char buffer[5120] = {0}; // 10 * 512 bytes
            struct EXT2DriverRequest read_request = {
                .name = entry->name,
                .name_len = entry->name_len,
                .parent_inode = find_cur_inode,
                .buf = buffer,
                .buffer_size = 5120,
                .is_directory = false
            };
            int32_t read_retcode = 0;
            syscall(0, (uint32_t)&read_request, (uint32_t)&read_retcode, 0);
            if (read_retcode != 0) {
                // error reading file
                print_string_colored("Error reading file: ", COLOR_RED);
                print_string_colored(entry->name, COLOR_RED);
                print_newline();
            }
  
            // Copy regular file
            struct EXT2DriverRequest copy_request = {
                .name = entry->name,
                .name_len = entry->name_len,
                .parent_inode = cur_dest_inode,
                .buf = &buffer,
                .buffer_size = 5120,
                .is_directory = false
            };

            int copy_retcode = 0;
            syscall(2, (uint32_t)&copy_request, (uint32_t)&copy_retcode, 0);

            if (copy_retcode == 0) {
                // Print copied file path
                char cur_copy_path[2048];
                int cur_copy_path_len = find_path_len;
                memcpy(cur_copy_path, find_path, find_path_len);
                cur_copy_path[cur_copy_path_len++] = '/';
                for (int i = 0; i < entry->name_len; i++) {
                    cur_copy_path[cur_copy_path_len++] = entry->name[i];
                }
                cur_copy_path[cur_copy_path_len] = '\0';
                
                print_string_colored("created : ", COLOR_LIGHT_BLUE);
                print_string_colored(cur_copy_path, COLOR_GREEN);
                print_newline();
            }
        }

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }
}