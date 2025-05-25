#include "header/usermode/commands/find.h"
#include "header/usermode/commands/cd.h"

char find_path[2048];
int find_path_len = 0;
int find_parent_inode = 0;
int find_cur_inode = 0;
char find_cur_directory[12];
char find_cur_directory_len = 0;

void find_recursive(char* str) {
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
        // error
        return;
    }

    struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;

    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 && entry->name_len == strlen(str) &&
            strcmp(entry->name, str) == 0) {
            // Found the entry

            char cur_find_path[2048];
            int cur_find_path_len = find_path_len;
            memcpy(cur_find_path, find_path, find_path_len);
            cur_find_path[cur_find_path_len++] = '/';

            for (int i = 0; i < entry->name_len; i++) {
                cur_find_path[cur_find_path_len++] = entry->name[i];
            }
            cur_find_path[cur_find_path_len] = '\0';
            
            if (entry->file_type == EXT2_FT_DIR) {
                print_string_colored(cur_find_path, COLOR_GREEN);
                cursor.row++;
                cursor.col = 0;
            } else {
                print_string_colored(cur_find_path, COLOR_WHITE);
                cursor.row++;
                cursor.col = 0;
            }
        }

        if (entry->file_type == EXT2_FT_DIR && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
          find_path[find_path_len++] = '/';
          for (int i = 0; i < entry->name_len; i++) {
            find_path[find_path_len++] = entry->name[i];
          }
          find_path[find_path_len] = '\0';

          char* bef_dir = find_cur_directory;
          int bef_dir_len = find_cur_directory_len;
          int bef_parent_inode = find_parent_inode;
          int bef_cur_inode = find_cur_inode;

          memcpy(find_cur_directory, entry->name, entry->name_len);
          find_cur_directory_len = entry->name_len;
          find_parent_inode = find_cur_inode;
          find_cur_inode = entry->inode;

          find_recursive(str);

          // Restore the path and directory info
          find_path_len -= entry->name_len + 1;
          find_path[find_path_len] = '\0';

          memcpy(find_cur_directory, bef_dir, bef_dir_len);
          find_cur_directory_len = bef_dir_len;
          find_parent_inode = bef_parent_inode;
          find_cur_inode = bef_cur_inode;
        }

        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
        }
    }
}


void find(char* str) {
  absolute_dir_info bef = DIR_INFO;

  cd_root();

  find_path[0] = '.';
  find_path_len = 1;
  find_parent_inode = (DIR_INFO.current_dir == 0) ? 1 : DIR_INFO.dir[DIR_INFO.current_dir - 1].inode;
  find_cur_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
  memcpy(find_cur_directory, DIR_INFO.dir[DIR_INFO.current_dir].dir_name, DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len);
  find_cur_directory_len = DIR_INFO.dir[DIR_INFO.current_dir].dir_name_len;

  find_recursive(str);

  DIR_INFO = bef; // Restore the original directory info
}

