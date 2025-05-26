#include "header/stdlib/strops.h"
#include "header/usermode/commands/rm.h"
#include "header/usermode/commands/cd.h"
#include "header/usermode/commands/find.h"

void rm(char *args1, char* args2, char* args3) {
  bool recursive = false;

  if (args3 != NULL && strlen(args3) > 0) {
    print_string_colored("Too many arguments\n", COLOR_RED);
    print_newline();

    print_string_colored("Usage: rm <filename> [--rf]", COLOR_LIGHT_RED);
    print_newline();

    return;
  }

  if (args2 != NULL && strcmp(args2, "--rf") == 0) {
    recursive = true;
  } else if (args2 != NULL && strlen(args2) > 0) {
    print_string_colored("Invalid option: ", COLOR_RED);
    print_string_colored(args2, COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (args1 == NULL || strlen(args1) == 0) {
    print_string_colored("Usage: rm <filename> [--rf]", COLOR_LIGHT_RED);
    print_newline();
    return;
  }

  if (strlen(args1) > 12) {
    print_string_colored("File name too long\n", COLOR_RED);
    print_newline();
    return;
  }
  
  if (!memcmp(args1, ".", 1) && strlen(args1) == 1) {
    print_string_colored("Cannot remove file with name '.'\n", COLOR_RED);
    print_newline();
    return;
  }

  if (!memcmp(args1, "..", 2) && strlen(args1) == 2) {
    print_string_colored("Cannot remove file with name '..'\n", COLOR_RED);
    print_newline();
    return;
  }

  if (strlen(args1) == 0) {
    print_string_colored("File name cannot be empty\n", COLOR_RED);
    print_newline();
    return;
  }

  if (!recursive) {
    struct EXT2DriverRequest request = {
      .name = args1,
      .name_len = strlen(args1),
      .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
      .buf = NULL,
      .buffer_size = 0, 
      .is_directory = false
    };

    int retcode = 0;
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 0) {
      print_string_colored("deleted : ", COLOR_LIGHT_BLUE);
      print_string_colored(args1, COLOR_GREEN);
      print_string_colored("File deleted successfully\n", COLOR_GREEN);
      print_newline();
    } else if (retcode == 1) {
      print_string_colored("File not found\n", COLOR_RED);
    } else if (retcode == 2) {
      print_string_colored("Directory is not empty\n", COLOR_RED);
    } else if (retcode == 3) {
      print_string_colored("Parent directory is invalid\n", COLOR_RED);
    } else {
      print_string_colored("Unknown error occurred\n", COLOR_RED);
    }
    print_newline();
  }
  else {
    struct EXT2DriverRequest request = {
      .name = args1,
      .name_len = strlen(args1),
      .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
      .buf = NULL,
      .buffer_size = 0, 
      .is_directory = false
    };

    int retcode = 0;
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);


    if (retcode == 0) {
      print_string_colored("deleted : ", COLOR_LIGHT_BLUE);
      print_string_colored(args1, COLOR_GREEN);
      print_string_colored("File deleted successfully\n", COLOR_GREEN);
      print_newline();
      return;
    }

    absolute_dir_info bef = DIR_INFO;

    //enter to the desired directory
    cd(args1);
    if (DIR_INFO.dir[DIR_INFO.current_dir].inode == bef.dir[bef.current_dir].inode) {
      return;
    }

    find_path[0] = '.';
    find_path_len = 1;
    find_parent_inode = (DIR_INFO.current_dir == 0) ? 1 : DIR_INFO.dir[DIR_INFO.current_dir - 1].inode;
    find_cur_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
    memcpy(find_cur_directory, DIR_INFO.dir[DIR_INFO.current_dir].dir_name, DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len);
    find_cur_directory_len = DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len;

    rm_recursive();

    DIR_INFO = bef;

    rm(args1, NULL, NULL);
  }
}

void rm_recursive() {
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

    // First pass: recursively delete subdirectories
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 && 
            entry->file_type == EXT2_FT_DIR && 
            strcmp(entry->name, ".") != 0 && 
            strcmp(entry->name, "..") != 0) {
            
            // Navigate into subdirectory
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

            // Recursively delete subdirectory contents
            rm_recursive();

            // Restore state
            find_path_len -= entry->name_len + 1;
            find_path[find_path_len] = '\0';
            memcpy(find_cur_directory, bef_dir, bef_dir_len);
            find_cur_directory_len = bef_dir_len;
            find_parent_inode = bef_parent_inode;
            find_cur_inode = bef_cur_inode;

            // Delete the now-empty subdirectory
            struct EXT2DriverRequest delete_request = {
                .name = entry->name,
                .name_len = entry->name_len,
                .parent_inode = find_cur_inode,
                .buf = NULL,
                .buffer_size = 0,
                .is_directory = true
            };
            int delete_retcode = 0;
            syscall(3, (uint32_t)&delete_request, (uint32_t)&delete_retcode, 0);
            
            if (delete_retcode == 0) {
                // Print deleted subdirectory path
                char cur_delete_path[2048];
                int cur_delete_path_len = find_path_len;
                memcpy(cur_delete_path, find_path, find_path_len);
                cur_delete_path[cur_delete_path_len++] = '/';
                for (int i = 0; i < entry->name_len; i++) {
                    cur_delete_path[cur_delete_path_len++] = entry->name[i];
                }
                cur_delete_path[cur_delete_path_len] = '\0';
                
                print_string_colored("deleted : ", COLOR_LIGHT_BLUE);
                print_string_colored(cur_delete_path, COLOR_GREEN);
                print_newline();
            }
        }

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }

    // Second pass: delete all files in current directory
    // Re-read directory since it may have changed
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0) {
        return;
    }
    
    entry = (struct EXT2DirectoryEntry*)request.buf;
    offset = 0;
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 && 
            entry->file_type != EXT2_FT_DIR &&
            strcmp(entry->name, ".") != 0 && 
            strcmp(entry->name, "..") != 0) {
            
            // Delete file
            struct EXT2DriverRequest delete_request = {
                .name = entry->name,
                .name_len = entry->name_len,
                .parent_inode = find_cur_inode,
                .buf = NULL,
                .buffer_size = 0,
                .is_directory = false
            };
            int delete_retcode = 0;
            syscall(3, (uint32_t)&delete_request, (uint32_t)&delete_retcode, 0);
            
            if (delete_retcode == 0) {
                // Print deleted file path
                char cur_delete_path[2048];
                int cur_delete_path_len = find_path_len;
                memcpy(cur_delete_path, find_path, find_path_len);
                cur_delete_path[cur_delete_path_len++] = '/';
                for (int i = 0; i < entry->name_len; i++) {
                    cur_delete_path[cur_delete_path_len++] = entry->name[i];
                }
                cur_delete_path[cur_delete_path_len] = '\0';
                
                print_string_colored("deleted : ", COLOR_LIGHT_BLUE);
                print_string_colored(cur_delete_path, COLOR_WHITE);
                print_newline();
            }
        }

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }
}