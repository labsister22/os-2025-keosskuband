#include <stdint.h>
#include <stdbool.h>
#include "header/filesys/ext2.h"
#include "header/stdlib/strops.h"
#include "header/usermode/user-shell.h"
#include "header/usermode/commands/apple.h"
#include "header/usermode/commands/help.h"
#include "header/usermode/commands/clear.h"
#include "header/usermode/commands/echo.h"
#include "header/usermode/commands/ls.h"
#include "header/usermode/commands/cd.h"
#include "header/usermode/commands/mkdir.h"
#include "header/usermode/commands/find.h"
#include "header/usermode/commands/cat.h"
#include "header/usermode/commands/touch.h"
#include "header/usermode/commands/ps.h"
#include "header/usermode/commands/kill.h"
#include "header/usermode/commands/exec.h"
#include "header/usermode/commands/ikuyokita.h"
#include "header/usermode/commands/rm.h"
#include "header/usermode/commands/cp.h"
#include "header/usermode/commands/mv.h"

static int previous_input_length = 0;

static CommandHistory history = {
    .count = 0,
    .current_index = -1
};

static ShellState shell_state = {
    .input_length = 0,
    .cursor_position = 0,
    .prompt_start_row = 0,
    .prompt_start_col = 0,
    .cursor_shown = false,
    .char_under_cursor = ' ',
    .char_color_under_cursor = COLOR_WHITE
};

// Tab completion state
static CompletionState completion_state = {
    .count = 0,
    .current_selection = 0,
    .is_showing = false
};

static const char* command_list[] = {
    "help", "clear", "echo", "ls", "cd", "mkdir", "find", "cat", 
    "exit", "apple", "touch", "rm", "cp", "mv", "ps", "size",
    "kill", "exec", "ikuyokita", "show_color", "sleep", NULL
};

static const char* rm_flags[] = {"--rf", NULL};
static const char* cp_flags[] = {"--rf", NULL};
static const char* pipeline_operators[] = {"|", NULL};

// cycling
static char original_prefix[MAX_COMPLETION_LENGTH] = "";
static int original_prefix_len = 0;
static int word_start_pos = 0;

CP cursor = {0, 0};
str_path path = {
    .path = "",
    .len = 0
};
absolute_dir_info DIR_INFO = {
    .current_dir = 0,
    .dir = {{".", 1, 1}}
};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

bool starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    
    int prefix_len = strlen((char*)prefix);
    int str_len = strlen((char*)str);
    
    if (prefix_len > str_len) return false;
    
    for (int i = 0; i < prefix_len; i++) {
        if (str[i] != prefix[i]) return false;
    }
    return true;
}

char* strchr_local(const char* str, int c) {
    if (!str) return NULL;
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return (*str == c) ? (char*)str : NULL;
}

void strcat_local(char* dest, const char* src) {
    if (!dest || !src) return;
    while (*dest) dest++;
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

bool is_absolute_path(const char* path) {
    return path && path[0] == '/';
}

bool is_relative_path(const char* path) {
    return path && (starts_with(path, "./") || starts_with(path, "../") || 
                   strcmp((char*)path, ".") == 0 || strcmp((char*)path, "..") == 0);
}

void split_path(const char* path, char* directory, char* filename) {
    directory[0] = '\0';
    filename[0] = '\0';
    
    if (!path || strlen((char*)path) == 0) return;
    
    int last_slash = -1;
    int path_len = strlen((char*)path);
    
    for (int i = path_len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }
    
    if (last_slash == -1) {
        strcpy(filename, (char*)path);
    } else {
        for (int i = 0; i < last_slash; i++) {
            directory[i] = path[i];
        }
        directory[last_slash] = '\0';
        
        strcpy(filename, (char*)(path + last_slash + 1));
    }
}

uint32_t resolve_path_inode(const char* path) {
    if (!path || strlen((char*)path) == 0) {
        return DIR_INFO.dir[DIR_INFO.current_dir].inode;
    }

    if (strcmp((char*)path, ".") == 0) {
        return DIR_INFO.dir[DIR_INFO.current_dir].inode;
    }
    
    if (strcmp((char*)path, "..") == 0) {
        return DIR_INFO.current_dir > 0 ? 
               DIR_INFO.dir[DIR_INFO.current_dir - 1].inode : 1;
    }

    absolute_dir_info temp_dir = DIR_INFO;
    
    char path_copy[MAX_PATH_LENGTH];
    strcpy(path_copy, (char*)path);
    
    char *token = path_copy;
    char *next_token;
    
    while (token != NULL) {
        next_token = strchr_local(token, '/');
        if (next_token != NULL) {
            *next_token = '\0';
            next_token++;
        }

        if (strlen(token) == 0) {
            token = next_token;
            continue;
        }
        
        if (strcmp(token, ".") == 0) {
        }

        else if (strcmp(token, "..") == 0) {
            if (temp_dir.current_dir > 0) {
                temp_dir.current_dir--;
            }
        }

        else {
            uint8_t dir_data[BLOCK_SIZE];
            struct EXT2DriverRequest request = {
                .buf = dir_data,
                .name = temp_dir.dir[temp_dir.current_dir].dir_name,
                .name_len = strlen(temp_dir.dir[temp_dir.current_dir].dir_name),
                .parent_inode = temp_dir.current_dir == 0 ? 1 : temp_dir.dir[temp_dir.current_dir - 1].inode,
                .buffer_size = BLOCK_SIZE,
                .is_directory = true
            };
            
            int32_t retcode = 0;
            syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
            
            if (retcode != 0) {
                return DIR_INFO.dir[DIR_INFO.current_dir].inode;
            }
            
            struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)request.buf;
            bool found = false;
            uint32_t offset = 0;
            
            while (offset < BLOCK_SIZE) {
                if (entry->inode != 0 && 
                    entry->name_len == strlen(token) &&
                    memcmp(entry->name, token, strlen(token)) == 0 &&
                    entry->file_type == EXT2_FT_DIR) {
                    found = true;
                    
                    if (temp_dir.current_dir < 49) {
                        temp_dir.current_dir++;
                        temp_dir.dir[temp_dir.current_dir].inode = entry->inode;
                        strcpy(temp_dir.dir[temp_dir.current_dir].dir_name, token);
                        temp_dir.dir[temp_dir.current_dir].dir_name_len = strlen(token);
                    }
                    break;
                }
                
                offset += entry->rec_len;
                if (offset < BLOCK_SIZE) {
                    entry = (struct EXT2DirectoryEntry*)((uint8_t*)request.buf + offset);
                }
            }
            
            if (!found) {
                return DIR_INFO.dir[DIR_INFO.current_dir].inode;
            }
        }
        
        token = next_token;
    }
    
    return temp_dir.dir[temp_dir.current_dir].inode;
}

bool flag_already_exists(const char* flag) {
    if (!flag) return false;
    
    int flag_len = strlen((char*)flag);
    if (flag_len == 0) return false;
    
    for (int i = 0; i <= shell_state.input_length - flag_len; i++) {
        if (i == 0 || shell_state.input_buffer[i - 1] == ' ') {
            bool matches = true;
            for (int j = 0; j < flag_len; j++) {
                if (shell_state.input_buffer[i + j] != flag[j]) {
                    matches = false;
                    break;
                }
            }
            
            if (matches) {
                int end_pos = i + flag_len;
                if (end_pos == shell_state.input_length || 
                    shell_state.input_buffer[end_pos] == ' ') {
                    return true;
                }
            }
        }
    }
    
    return false;
}

arg_type_t get_command_arg_type(const char* command, int arg_position) {
    if (strcmp((char*)command, "help") == 0) return ARG_NONE;
    if (strcmp((char*)command, "clear") == 0) return ARG_NONE;
    if (strcmp((char*)command, "cls") == 0) return ARG_NONE;
    if (strcmp((char*)command, "ps") == 0) return ARG_NONE;
    if (strcmp((char*)command, "ikuyokita") == 0) return ARG_NONE;
    if (strcmp((char*)command, "show_color") == 0) return ARG_NONE;
    if (strcmp((char*)command, "apple") == 0) return ARG_NONE;
    if (strcmp((char*)command, "exit") == 0) return ARG_NONE;

    if (strcmp((char*)command, "echo") == 0) return ARG_TEXT;
    if (strcmp((char*)command, "find") == 0) return ARG_TEXT;  
    if (strcmp((char*)command, "mkdir") == 0) return ARG_NEW_NAME;  
    if (strcmp((char*)command, "touch") == 0) return ARG_NEW_NAME;  
    if (strcmp((char*)command, "sleep") == 0) return ARG_INTEGER;   
    if (strcmp((char*)command, "kill") == 0) return ARG_INTEGER;    
    
    if (strcmp((char*)command, "cat") == 0) return ARG_EXISTING_FILE;
    if (strcmp((char*)command, "exec") == 0) return ARG_EXISTING_FILE;

    if (strcmp((char*)command, "cd") == 0) return ARG_EXISTING_DIR;
    if (strcmp((char*)command, "ls") == 0) return ARG_EXISTING_DIR;  

    if (strcmp((char*)command, "rm") == 0) {
        if (arg_position == 0) return ARG_EXISTING_ANY;

        else if (flag_already_exists("--rf")) return ARG_NONE;
        else return ARG_NONE;
    }

    if (strcmp((char*)command, "cp") == 0) {
        if (arg_position == 0) return ARG_EXISTING_ANY;  // source
        else if (arg_position == 1) return ARG_NEW_NAME;  // destination

        else if (flag_already_exists("--rf")) return ARG_NONE;
        else return ARG_NONE;
    }
    
    if (strcmp((char*)command, "mv") == 0) {
        if (arg_position == 0) return ARG_EXISTING_ANY;  // source  
        else if (arg_position == 1) return ARG_NEW_NAME;  // destination
        else return ARG_NONE;
    }
    
    return ARG_NONE;
}

int get_current_arg_position(const char* command) {
    (void)command;  // Suppress unused parameter warning
    int arg_pos = 0;
    
    int cmd_end = 0;
    for (int i = 0; i < shell_state.input_length; i++) {
        if (shell_state.input_buffer[i] == ' ') {
            cmd_end = i;
            break;
        }
    }
    
    if (cmd_end == 0) cmd_end = shell_state.input_length;
    
    if (shell_state.cursor_position <= cmd_end) {
        return -1;
    }
    
    for (int i = cmd_end; i < shell_state.cursor_position; i++) {
        if (shell_state.input_buffer[i] == ' ') {
            while (i < shell_state.cursor_position && shell_state.input_buffer[i] == ' ') {
                i++;
            }

            if (i < shell_state.cursor_position && shell_state.input_buffer[i] != ' ') {
                arg_pos++;

                while (i < shell_state.cursor_position && shell_state.input_buffer[i] != ' ') {
                    i++;
                }
                i--;
            } else if (i >= shell_state.cursor_position) {
                break;
            }
        }
    }
    
    return arg_pos;
}

completion_context analyze_completion_context() {
    completion_context ctx = {0};
    
    ctx.word_start = shell_state.cursor_position;
    while (ctx.word_start > 0 && shell_state.input_buffer[ctx.word_start - 1] != ' ') {
        ctx.word_start--;
    }

    int word_len = shell_state.cursor_position - ctx.word_start;
    if (word_len > 0 && word_len < MAX_COMPLETION_LENGTH) {
        for (int i = 0; i < word_len; i++) {
            ctx.current_word[i] = shell_state.input_buffer[ctx.word_start + i];
        }
    }
    ctx.current_word[word_len] = '\0';

    int cmd_end = 0;
    bool found_pipeline = false;
    int pipeline_pos = -1;

    for (int i = 0; i < shell_state.input_length; i++) {
        if (shell_state.input_buffer[i] == ' ' && cmd_end == 0) {
            cmd_end = i;
        }
        if (shell_state.input_buffer[i] == '|') {
            found_pipeline = true;
            pipeline_pos = i;
            break;
        }
    }
    
    if (cmd_end == 0) cmd_end = shell_state.input_length;

    int cmd_len = (cmd_end < MAX_ARGS_LENGTH - 1) ? cmd_end : MAX_ARGS_LENGTH - 1;
    for (int i = 0; i < cmd_len; i++) {
        ctx.command[i] = shell_state.input_buffer[i];
    }
    ctx.command[cmd_len] = '\0';
    
    ctx.after_pipeline = found_pipeline && (shell_state.cursor_position > pipeline_pos);
    
    ctx.is_flag = (ctx.current_word[0] == '-');
    
    ctx.is_path = (strchr_local(ctx.current_word, '/') != NULL || 
                   strcmp(ctx.current_word, ".") == 0 || 
                   strcmp(ctx.current_word, "..") == 0 ||
                   starts_with(ctx.current_word, "./") ||
                   starts_with(ctx.current_word, "../"));
    
    if (ctx.after_pipeline) {
        if (ctx.word_start == pipeline_pos + 1 || 
            (ctx.word_start > pipeline_pos + 1 && 
             shell_state.input_buffer[pipeline_pos + 1] == ' ')) {
            ctx.context = COMPLETION_PIPELINE;
        } else {
            char piped_cmd[MAX_ARGS_LENGTH] = {0};
            int pipe_cmd_start = pipeline_pos + 1;
            while (pipe_cmd_start < shell_state.input_length && shell_state.input_buffer[pipe_cmd_start] == ' ') {
                pipe_cmd_start++;
            }
            int pipe_cmd_end = pipe_cmd_start;
            while (pipe_cmd_end < shell_state.input_length && shell_state.input_buffer[pipe_cmd_end] != ' ') {
                pipe_cmd_end++;
            }
            for (int i = pipe_cmd_start; i < pipe_cmd_end && i - pipe_cmd_start < MAX_ARGS_LENGTH - 1; i++) {
                piped_cmd[i - pipe_cmd_start] = shell_state.input_buffer[i];
            }
            
            int pipe_arg_pos = get_current_arg_position(piped_cmd);
            arg_type_t pipe_arg_type = get_command_arg_type(piped_cmd, pipe_arg_pos);
            
            if (pipe_arg_type == ARG_EXISTING_FILE || pipe_arg_type == ARG_EXISTING_DIR || pipe_arg_type == ARG_EXISTING_ANY) {
                ctx.context = COMPLETION_FILE;
            } else {
                ctx.context = COMPLETION_COMMAND;
            }
        }
    } else if (shell_state.cursor_position <= cmd_end) {
        ctx.context = COMPLETION_COMMAND;
    } else if (ctx.is_flag) {
        ctx.context = COMPLETION_FLAG;
    } else if (ctx.is_path) {
        ctx.context = COMPLETION_PATH;
    } else {
        int arg_pos = get_current_arg_position(ctx.command);
        arg_type_t arg_type = get_command_arg_type(ctx.command, arg_pos);
        
        switch (arg_type) {
            case ARG_EXISTING_FILE:
            case ARG_EXISTING_ANY:
                ctx.context = COMPLETION_FILE;
                break;
            case ARG_EXISTING_DIR:
                ctx.context = COMPLETION_DIRECTORY;
                break;
            case ARG_NONE:
            case ARG_TEXT:
            case ARG_INTEGER:
            case ARG_NEW_NAME:
            default:
                if ((strcmp(ctx.command, "rm") == 0 || strcmp(ctx.command, "cp") == 0) && 
                    arg_pos >= 1 && strlen(ctx.current_word) == 0) {
                    if (!flag_already_exists("--rf")) {
                        ctx.context = COMPLETION_FLAG;
                    } else {
                        ctx.context = COMPLETION_COMMAND;
                    }
                } else {
                    ctx.context = COMPLETION_COMMAND;
                }
                break;
        }
    }
    
    return ctx;
}

void find_command_completions(const char* prefix) {
    completion_state.count = 0;
    
    for (int i = 0; command_list[i] != NULL && completion_state.count < MAX_COMPLETIONS; i++) {
        if (starts_with(command_list[i], prefix)) {
            strcpy(completion_state.completions[completion_state.count], (char*)command_list[i]);
            completion_state.count++;
        }
    }
}

void find_flag_completions(const char* command, const char* prefix) {
    completion_state.count = 0;
    
    const char** flags = NULL;
    
    if (strcmp((char*)command, "rm") == 0) {
        flags = rm_flags;
    } else if (strcmp((char*)command, "cp") == 0) {
        flags = cp_flags;
    }
    
    if (flags) {
        for (int i = 0; flags[i] != NULL && completion_state.count < MAX_COMPLETIONS; i++) {
            if (starts_with(flags[i], prefix)) {
                strcpy(completion_state.completions[completion_state.count], (char*)flags[i]);
                completion_state.count++;
            }
        }
    }
}

void find_pipeline_completions(const char* prefix) {
    completion_state.count = 0;

    for (int i = 0; pipeline_operators[i] != NULL && completion_state.count < MAX_COMPLETIONS; i++) {
        if (starts_with(pipeline_operators[i], prefix)) {
            strcpy(completion_state.completions[completion_state.count], (char*)pipeline_operators[i]);
            completion_state.count++;
        }
    }

    find_command_completions(prefix);
}

void find_smart_file_completions(const char* command, const char* prefix) {
    completion_state.count = 0;
    
    int arg_pos = get_current_arg_position(command);
    arg_type_t arg_type = get_command_arg_type(command, arg_pos);
    
    if (arg_type == ARG_NONE || arg_type == ARG_TEXT || 
        arg_type == ARG_INTEGER || arg_type == ARG_NEW_NAME) {
        return;
    }
    
    char directory[MAX_PATH_LENGTH] = "";
    char filename[MAX_COMPLETION_LENGTH] = "";
    uint32_t target_inode;
    
    if (strchr_local(prefix, '/') != NULL) {
        split_path(prefix, directory, filename);
        target_inode = resolve_path_inode(directory);
    } else {
        strcpy(filename, (char*)prefix);
        target_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
    }
    
    uint8_t dir_buffer[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_buffer,
        .name = strlen(directory) == 0 ? "." : directory,
        .parent_inode = target_inode,
        .buffer_size = BLOCK_SIZE,
        .name_len = strlen(directory) == 0 ? 1 : strlen(directory),
        .is_directory = false
    };
    
    int8_t result;
    syscall(1, (uint32_t)&request, (uint32_t)&result, 0);
    
    if (result != 0) return;
    
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE && completion_state.count < MAX_COMPLETIONS) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry*)(dir_buffer + offset);
        
        if (entry->rec_len == 0) break;
        if (entry->inode == 0) {
            offset += entry->rec_len;
            continue;
        }
        
        char entry_name[256];
        for (int i = 0; i < entry->name_len && i < 255; i++) {
            entry_name[i] = entry->name[i];
        }
        entry_name[entry->name_len] = '\0';
        
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            if (arg_type != ARG_EXISTING_DIR) {
                offset += entry->rec_len;
                continue;
            }
        }

        bool include = false;
        switch (arg_type) {
            case ARG_EXISTING_FILE:
                include = (entry->file_type == EXT2_FT_REG_FILE);
                break;
            case ARG_EXISTING_DIR:
                include = (entry->file_type == EXT2_FT_DIR);
                break;
            case ARG_EXISTING_ANY:
                include = true;
                break;
            default:
                include = false;
                break;
        }
        
        if (include && starts_with(entry_name, filename)) {
            char full_completion[MAX_COMPLETION_LENGTH];
            if (strlen(directory) > 0) {
                strcpy(full_completion, directory);
                strcat_local(full_completion, "/");
                strcat_local(full_completion, entry_name);
            } else {
                strcpy(full_completion, entry_name);
            }
            
            if (strlen(full_completion) < MAX_COMPLETION_LENGTH) {
                strcpy(completion_state.completions[completion_state.count], full_completion);
                completion_state.count++;
            }
        }
        
        offset += entry->rec_len;
    }
}

void find_directory_completions(const char* prefix) {
    find_smart_file_completions("cd", prefix);
}

void find_file_completions(const char* command, const char* prefix) {
    find_smart_file_completions(command, prefix);
}

void find_path_completions(const char* path_input) {
    completion_state.count = 0;
    
    char directory[MAX_PATH_LENGTH];
    char filename[MAX_COMPLETION_LENGTH];
    
    split_path(path_input, directory, filename);
    
    uint32_t target_inode;
    if (strlen(directory) == 0) {
        target_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode;
    } else {
        target_inode = resolve_path_inode(directory);
    }

    uint8_t dir_buffer[BLOCK_SIZE];
    struct EXT2DriverRequest request = {
        .buf = dir_buffer,
        .name = strlen(directory) == 0 ? "." : directory,
        .parent_inode = target_inode,
        .buffer_size = BLOCK_SIZE,
        .name_len = strlen(directory) == 0 ? 1 : strlen(directory),
        .is_directory = false
    };
    
    int8_t result;
    syscall(1, (uint32_t)&request, (uint32_t)&result, 0);
    
    if (result != 0) return;

    uint32_t offset = 0;
    while (offset < BLOCK_SIZE && completion_state.count < MAX_COMPLETIONS) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry*)(dir_buffer + offset);
        
        if (entry->rec_len == 0) break;
        if (entry->inode == 0) {
            offset += entry->rec_len;
            continue;
        }
        
        char entry_name[256];
        for (int i = 0; i < entry->name_len && i < 255; i++) {
            entry_name[i] = entry->name[i];
        }
        entry_name[entry->name_len] = '\0';
        
        if (starts_with(entry_name, filename)) {
            char full_completion[MAX_COMPLETION_LENGTH];
            if (strlen(directory) > 0) {
                strcpy(full_completion, directory);
                strcat_local(full_completion, "/");
                strcat_local(full_completion, entry_name);
            } else {
                strcpy(full_completion, entry_name);
            }
            
            if (strlen(full_completion) < MAX_COMPLETION_LENGTH) {
                strcpy(completion_state.completions[completion_state.count], full_completion);
                completion_state.count++;
            }
        }
        
        offset += entry->rec_len;
    }
}

void apply_current_completion() {
    if (completion_state.count == 0) return;
    
    hide_cursor();
    
    const char* completion = completion_state.completions[completion_state.current_selection];
    int completion_len = strlen((char*)completion);

    int current_word_len = shell_state.cursor_position - word_start_pos;
    for (int i = word_start_pos; i < shell_state.input_length - current_word_len; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + current_word_len];
    }
    shell_state.input_length -= current_word_len;
    shell_state.cursor_position = word_start_pos;
    
    if (shell_state.input_length + completion_len < MAX_INPUT_LENGTH - 1) {
        for (int i = shell_state.input_length + completion_len - 1; i >= word_start_pos + completion_len; i--) {
            shell_state.input_buffer[i] = shell_state.input_buffer[i - completion_len];
        }

        for (int i = 0; i < completion_len; i++) {
            shell_state.input_buffer[word_start_pos + i] = completion[i];
        }
        
        shell_state.input_length += completion_len;
        shell_state.cursor_position += completion_len;
        shell_state.input_buffer[shell_state.input_length] = '\0';

        if (shell_state.cursor_position == shell_state.input_length && 
            shell_state.input_length < MAX_INPUT_LENGTH - 1) {
            shell_state.input_buffer[shell_state.input_length] = ' ';
            shell_state.input_length++;
            shell_state.cursor_position++;
            shell_state.input_buffer[shell_state.input_length] = '\0';
        }
    }
    
    redraw_input_line();
}

void reset_completion_state() {
    completion_state.count = 0;
    completion_state.current_selection = 0;
    completion_state.is_showing = false;
    original_prefix[0] = '\0';
    original_prefix_len = 0;
}

void handle_tab_completion() {
    if (completion_state.is_showing && completion_state.count > 0) {
        completion_state.current_selection = (completion_state.current_selection + 1) % completion_state.count;
        apply_current_completion();
        return;
    }

    reset_completion_state();
    
    if (shell_state.input_length == 0) {
        find_command_completions("");
        if (completion_state.count > 0) {
            word_start_pos = 0;
            strcpy(original_prefix, "");
            original_prefix_len = 0;
            completion_state.is_showing = true;
            apply_current_completion();
        }
        return;
    }

    completion_context ctx = analyze_completion_context();
    word_start_pos = ctx.word_start;
    strcpy(original_prefix, ctx.current_word);
    original_prefix_len = strlen(original_prefix);

    switch (ctx.context) {
        case COMPLETION_COMMAND:
            {
                int arg_pos_check = get_current_arg_position(ctx.command);
                if (arg_pos_check == -1) {
                    find_command_completions(ctx.current_word);
                } else {
                    completion_state.count = 0;
                }
            }
            break;
            
        case COMPLETION_FLAG:
            find_flag_completions(ctx.command, ctx.current_word);
            break;
            
        case COMPLETION_DIRECTORY:
            find_directory_completions(ctx.current_word);
            break;
            
        case COMPLETION_FILE:
            find_file_completions(ctx.command, ctx.current_word);
            break;
            
        case COMPLETION_PATH:
            find_path_completions(ctx.current_word);
            break;
            
        case COMPLETION_PIPELINE:
            find_pipeline_completions(ctx.current_word);
            break;
            
        default:
            completion_state.count = 0;
            break;
    }

    if (completion_state.count > 0) {
        completion_state.is_showing = true;
        apply_current_completion();
    }
}

void add_to_history(const char* command) {
    if (strlen((char*)command) == 0) {
        return;
    }

    if (history.count > 0) {
        int most_recent_index;
        if (history.count < MAX_HISTORY_ENTRIES) {
            most_recent_index = history.count - 1;
        } else {
            most_recent_index = (history.count - 1) % MAX_HISTORY_ENTRIES;
        }
        
        if (strcmp(history.commands[most_recent_index], (char*)command) == 0) {
            return;
        }
    }

    if (history.count < MAX_HISTORY_ENTRIES) {
        strcpy(history.commands[history.count], (char*)command);
        history.count++;
    } else {
        for (int i = 0; i < MAX_HISTORY_ENTRIES - 1; i++) {
            strcpy(history.commands[i], history.commands[i + 1]);
        }

        strcpy(history.commands[MAX_HISTORY_ENTRIES - 1], (char*)command);
    }

    history.current_index = -1;
}

void load_history_entry(int index) {
    if (index < 0 || index >= history.count) {
        return;
    }

    int old_length = shell_state.input_length;
    
    clear_input_buffer();

    strcpy(shell_state.input_buffer, history.commands[index]);
    shell_state.input_length = strlen(shell_state.input_buffer);
    shell_state.cursor_position = shell_state.input_length;

    previous_input_length = old_length;
    
    redraw_input_line();
}

void handle_up_arrow() {
    if (history.count == 0) {
        return;
    }
    
    if (history.current_index == -1) {
        history.current_index = history.count - 1;
    } else if (history.current_index > 0) {
        history.current_index--;
    } else {
        return;
    }
    
    load_history_entry(history.current_index);
}

void handle_down_arrow() {
    if (history.count == 0 || history.current_index == -1) {
        return;
    }
    
    if (history.current_index < history.count - 1) {
        history.current_index++;
        load_history_entry(history.current_index);
    } else {
        history.current_index = -1;

        int old_length = shell_state.input_length;
        previous_input_length = old_length;
        
        clear_input_buffer();
        redraw_input_line();
    }
}

// Graphics cursor syscalls
void graphics_draw_cursor_syscall() {
    syscall(10, 0, 0, 0);
}

void graphics_erase_cursor_syscall() {
    syscall(11, 0, 0, 0);
}

void graphics_store_char_syscall(char c, uint8_t color) {
    syscall(12, (uint32_t)c, (uint32_t)color, 0);
}

void graphics_set_cursor_colors_syscall(uint8_t fg_color, uint8_t bg_color) {
    syscall(15, (uint32_t)fg_color, (uint32_t)bg_color, 0);
}

// Cursor management functions
void show_cursor() {
    if (!shell_state.cursor_shown) {
        // Make sure cursor screen position is up to date
        update_cursor_row_col();
        
        // Store the character that's currently at the cursor position
        if (shell_state.cursor_position < shell_state.input_length) {
            shell_state.char_under_cursor = shell_state.input_buffer[shell_state.cursor_position];
            shell_state.char_color_under_cursor = COLOR_WHITE;
        } else {
            shell_state.char_under_cursor = ' ';
            shell_state.char_color_under_cursor = COLOR_BLACK;
        }

        // Set cursor colors and draw
        graphics_set_cursor_colors_syscall(COLOR_WHITE, COLOR_GREEN);
        graphics_store_char_syscall(shell_state.char_under_cursor, shell_state.char_color_under_cursor);
        graphics_draw_cursor_syscall();
        shell_state.cursor_shown = true;
    }
}

void hide_cursor() {
    if (shell_state.cursor_shown) {
        graphics_erase_cursor_syscall();
        
        // Calculate the correct screen position for this logical cursor position
        int total_pos = shell_state.prompt_start_col + shell_state.cursor_position;
        int screen_row = shell_state.prompt_start_row + total_pos / SCREEN_WIDTH;
        int screen_col = total_pos % SCREEN_WIDTH;
        
        // Restore the correct character at the calculated position
        char char_to_restore;
        uint8_t color_to_restore;
        
        if (shell_state.cursor_position < shell_state.input_length) {
            char_to_restore = shell_state.input_buffer[shell_state.cursor_position];
            color_to_restore = COLOR_WHITE;
        } else {
            char_to_restore = ' ';
            color_to_restore = COLOR_BLACK;
        }
        
        CP pos = {screen_row, screen_col};
        syscall(5, (uint32_t)&char_to_restore, color_to_restore, (uint32_t)&pos);
        
        shell_state.cursor_shown = false;
    }
}

void clear_input_buffer() {
    for (int i = 0; i < MAX_INPUT_LENGTH; i++) {
        shell_state.input_buffer[i] = '\0';
    }
    shell_state.input_length = 0;
    shell_state.cursor_position = 0;
}

void set_hardware_cursor() {
    uint16_t pixel_x = cursor.col * 5;
    uint16_t pixel_y = cursor.row * 8;
    syscall(8, pixel_x, pixel_y, 0);
}

void scroll_screen() {
    syscall(18, 8, COLOR_BLACK, 0); // Scroll 1 line up
    set_hardware_cursor();
}

void check_and_scroll() {
    while (cursor.row >= SCREEN_HEIGHT) {
        scroll_screen();
        if (shell_state.prompt_start_row > 0) {
            shell_state.prompt_start_row--;
        }
        cursor.row--;
    }
}

void advance_cursor() {
    cursor.col++;

    if (cursor.col >= SCREEN_WIDTH) {
        cursor.col = 0;
        cursor.row++;
    }

    check_and_scroll();
    set_hardware_cursor();
}

void print_string_colored(const char* str, uint8_t color) {
    hide_cursor();

    int i = 0;
    while (str[i] != '\0') {
        check_and_scroll();
        syscall(5, (uint32_t)&str[i], color, (uint32_t)&cursor);
        advance_cursor();
        i++;
    }
}

void print_string_at_cursor(const char* str) {
    print_string_colored(str, COLOR_WHITE);
}

void print_newline() {
    hide_cursor();

    cursor.col = 0;
    cursor.row++;

    check_and_scroll();
    set_hardware_cursor();
}

void update_cursor_row_col() {
    hide_cursor();
    int total_pos = shell_state.prompt_start_col + shell_state.cursor_position;
    cursor.row = shell_state.prompt_start_row + total_pos / SCREEN_WIDTH;
    cursor.col = total_pos % SCREEN_WIDTH;

    check_and_scroll();
    set_hardware_cursor();
}

void print_prompt() {
    check_and_scroll();
    
    print_string_colored(SHELL_PROMPT, COLOR_LIGHT_CYAN);
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    for (int i = 0; i <= DIR_INFO.current_dir; i++) {
        for (int j = 0; j < DIR_INFO.dir[i].dir_name_len; j++) {
            syscall(5, (uint32_t)&DIR_INFO.dir[i].dir_name[j], COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
            if (cursor.col >= SCREEN_WIDTH) {
                cursor.col = 0;
                cursor.row++;
                check_and_scroll();
            }
        }
        if (i < DIR_INFO.current_dir) {
            syscall(5, (uint32_t)"/", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
            cursor.col++;
            if (cursor.col >= SCREEN_WIDTH) {
                cursor.col = 0;
                cursor.row++;
                check_and_scroll();
            }
        }
    }

    print_newline();
    check_and_scroll();
    
    syscall(5, (uint32_t)":", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;
    syscall(5, (uint32_t)"$", COLOR_LIGHT_CYAN, (uint32_t)&cursor);
    cursor.col++;

    // Save prompt start row/col for input text (cursor after prompt)
    shell_state.prompt_start_row = cursor.row;
    shell_state.prompt_start_col = cursor.col;

    update_cursor_row_col();
    set_hardware_cursor();
    show_cursor();
}

void redraw_input_line() {
    hide_cursor();

    // Calculate area to clear based on MAXIMUM of current and previous input length
    int max_input_length = (shell_state.input_length > previous_input_length) ? 
                          shell_state.input_length : previous_input_length;
    
    int total_chars = shell_state.prompt_start_col + max_input_length;
    int total_rows = (total_chars + SCREEN_WIDTH - 1) / SCREEN_WIDTH;
    int end_row = shell_state.prompt_start_row + total_rows - 1;

    while (end_row >= SCREEN_HEIGHT) {
        scroll_screen();
        shell_state.prompt_start_row--;
        end_row--;
    }

    // Clear the entire area that might have been used by previous input
    for (int row = shell_state.prompt_start_row; row <= end_row && row < SCREEN_HEIGHT; row++) {
        int start_col = (row == shell_state.prompt_start_row) ? shell_state.prompt_start_col : 0;
        for (int col = start_col; col < SCREEN_WIDTH; col++) {
            char space = ' ';
            CP pos = {row, col};
            syscall(5, (uint32_t)&space, COLOR_BLACK, (uint32_t)&pos);
        }
    }

    // Draw the current input
    int draw_row = shell_state.prompt_start_row;
    int draw_col = shell_state.prompt_start_col;
    for (int i = 0; i < shell_state.input_length; i++) {
        CP pos = {draw_row, draw_col};
        syscall(5, (uint32_t)&shell_state.input_buffer[i], COLOR_WHITE, (uint32_t)&pos);
        draw_col++;
        if (draw_col >= SCREEN_WIDTH) {
            draw_col = 0;
            draw_row++;
            if (draw_row >= SCREEN_HEIGHT) {
                draw_row = SCREEN_HEIGHT - 1;
            }
        }
    }

    previous_input_length = shell_state.input_length;

    update_cursor_row_col();
    set_hardware_cursor();
    show_cursor();
}

void insert_char_at_cursor(char c) {
    if (shell_state.input_length >= MAX_INPUT_LENGTH - 1) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.input_length; i > shell_state.cursor_position; i--) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i - 1];
    }

    shell_state.input_buffer[shell_state.cursor_position] = c;
    shell_state.input_length++;
    shell_state.cursor_position++;
    shell_state.input_buffer[shell_state.input_length] = '\0';
    
    // Update previous length tracking
    previous_input_length = shell_state.input_length;
}

void delete_char_before_cursor() {
    if (shell_state.cursor_position <= 0 || shell_state.input_length <= 0) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.cursor_position - 1; i < shell_state.input_length - 1; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + 1];
    }

    shell_state.input_length--;
    shell_state.cursor_position--;
    shell_state.input_buffer[shell_state.input_length] = '\0';
    
    update_cursor_row_col();
}

void delete_char_at_cursor() {
    if (shell_state.cursor_position >= shell_state.input_length) {
        return;
    }

    hide_cursor();

    for (int i = shell_state.cursor_position; i < shell_state.input_length - 1; i++) {
        shell_state.input_buffer[i] = shell_state.input_buffer[i + 1];
    }

    shell_state.input_length--;
    shell_state.input_buffer[shell_state.input_length] = '\0';

    update_cursor_row_col();
}

void move_cursor_left() {
    if (shell_state.cursor_position > 0) {
        shell_state.cursor_position--;
        redraw_input_line();
    }
}

void move_cursor_right() {
    if (shell_state.cursor_position < shell_state.input_length) {
        shell_state.cursor_position++;
        redraw_input_line();
    }
}

void process_command() {
    hide_cursor();
    print_newline();

    if (shell_state.input_length > 0) {
        add_to_history(shell_state.input_buffer);
        
        for (int j = 0; j < MAX_ARGS_AMOUNT; j++) {
            for (int k = 0; k < MAX_ARGS_LENGTH; k++) {
                shell_state.args[j][k] = '\0';
            }
        }

        int i = 0;
        // get command
        while (i < shell_state.input_length && shell_state.input_buffer[i] != ' ' &&
               shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r') {
               
           shell_state.command[i] = shell_state.input_buffer[i];   
           i++;
        }
        shell_state.command[i] = '\0';

        if (strcmp(shell_state.command, "echo") == 0) {
            while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
                i++;

            int text_start = i;
            int text_length = 0;
            //find the args to be "echo-ed"
            while (i < shell_state.input_length && shell_state.input_buffer[i] != '|' &&
                shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r') {
                text_length++;
                i++;
            }

            int args_idx = 0;
            int args_buffer_idx = 0;
            int args_used_amount = 0;

            while (i < shell_state.input_length && args_idx < MAX_ARGS_AMOUNT) {
                while (i < shell_state.input_length && shell_state.input_buffer[i] != ' ' &&
                    shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r' &&
                    args_buffer_idx < MAX_ARGS_LENGTH - 1) {
                    shell_state.args[args_idx][args_buffer_idx] = shell_state.input_buffer[i];
                    args_buffer_idx++;
                    i++;
                }

                if (args_buffer_idx > 0) {
                    shell_state.args[args_idx][args_buffer_idx] = '\0'; // Null terminate
                    args_buffer_idx = 0;
                    args_idx++;
                    args_used_amount++;
                }

                // skip whitespace
                while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
                    i++;
            }

            if (args_used_amount == 0) {
                echo(shell_state.input_buffer + text_start, text_length);
            } else if (args_used_amount == 3 && 
                       strcmp(shell_state.args[0], "|") == 0 &&
                       strcmp(shell_state.args[1], "touch") == 0 ) {
                // Special case for echo to file
                shell_state.input_buffer[text_start + text_length] = '\0'; // Null terminate the echo text
                touch(shell_state.args[2], shell_state.input_buffer + text_start, text_length);
            } else {
                print_string_colored("Usage: [echo <text>] or [echo <text> | touch <filename>]", COLOR_LIGHT_RED);
                print_newline();
            }

            return;
        }
            

        // skip the whitespaces to get to first args
        while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
            i++;

        int args_idx = 0;
        int args_buffer_idx = 0;
        int args_used_amount = 0;
        while (i < shell_state.input_length && args_idx < MAX_ARGS_AMOUNT) {
            while (i < shell_state.input_length && shell_state.input_buffer[i] != ' ' &&
                   shell_state.input_buffer[i] != '\n' && shell_state.input_buffer[i] != '\r' &&
                   args_buffer_idx < MAX_ARGS_LENGTH - 1) {
                shell_state.args[args_idx][args_buffer_idx] = shell_state.input_buffer[i];
                args_buffer_idx++;
                i++;
            }

            if (args_buffer_idx > 0) {
                shell_state.args[args_idx][args_buffer_idx] = '\0'; // Null terminate
                args_buffer_idx = 0;
                args_idx++;
                args_used_amount++;
            }

            // skip whitespace
            while (i < shell_state.input_length && shell_state.input_buffer[i] == ' ')
                i++;
        }

        // do commands using strcmp from strops.h
        if (strcmp("help", shell_state.command) == 0) {
            help();
        } else if (strcmp("clear", shell_state.command) == 0) {
            clear();
        } else if (strcmp("ls", shell_state.command) == 0) {
            ls(shell_state.args[0]);
        }
        else if (strcmp("cd", shell_state.command) == 0) {
            cd(shell_state.args[0]);
        }
        else if (strcmp("mkdir", shell_state.command) == 0) {
            mkdir(shell_state.args[0]);
        }
        else if (strcmp("find", shell_state.command) == 0) {
            if (args_used_amount > 0) {
                find(shell_state.args[0]);
            } else {
                print_string_colored("Usage: find <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (strcmp("size", shell_state.command) == 0) {
            if (args_used_amount == 1) {
                struct EXT2DriverRequest request = {
                    .buf = NULL,
                    .name = shell_state.args[0],
                    .parent_inode = DIR_INFO.dir[DIR_INFO.current_dir].inode,
                    .buffer_size = 0,
                    .name_len = strlen(shell_state.args[0]),
                };
                
                int size = 0;
                syscall(21, (uint32_t)&request, (uint32_t)&size, 0);

                print_string_colored("Size of '", COLOR_LIGHT_CYAN);
                print_string_colored(shell_state.args[0], COLOR_WHITE);
                print_string_colored("' is ", COLOR_LIGHT_CYAN);
                if (size < 0) {
                    print_string_colored("unknown", COLOR_LIGHT_RED);
                } else {
                    char size_str[20];
                    int_toString(size, size_str);
                    print_string_colored(size_str, COLOR_WHITE);
                    print_string_colored(" bytes.", COLOR_LIGHT_CYAN);
                }
                print_newline();
            } else {
                print_string_colored("Usage: size <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (!strcmp("cat", shell_state.command)) {
            if (args_used_amount > 0) {
                cat(shell_state.args[0]);
            } else {
                print_string_colored("Usage: cat <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (!strcmp("touch", shell_state.command)) {
            if (args_used_amount > 0) {
                touch(shell_state.args[0], NULL, 0);
            } else {
                print_string_colored("Usage: touch <filename>", COLOR_LIGHT_RED);
                print_newline();
            }
        } else if (strcmp("rm", shell_state.command) == 0) {
            rm(shell_state.args[0], shell_state.args[1], shell_state.args[2]);
        } else if (strcmp("cp", shell_state.command) == 0) {
            cp(shell_state.args[0], shell_state.args[1], shell_state.args[2], shell_state.args[3]);
        } else if (strcmp("mv", shell_state.command) == 0) {
            mv(shell_state.args[0], shell_state.args[1], shell_state.args[2]);
        } else if (strcmp("exit", shell_state.command) == 0) {
            print_string_at_cursor("Goodbye!");
            print_newline();
            while (true) {}
        } else if (strcmp("apple", shell_state.command) == 0) {
            apple(&cursor);
        } else if (strcmp("ikuyokita", shell_state.command) == 0) {
            ikuyokita();
        } else if (strcmp("ps", shell_state.command) == 0) {
            ps();
        } else if (strcmp("kill", shell_state.command) == 0) {
            kill(shell_state.args[0]);
        } else if (strcmp("exec", shell_state.command) == 0) {
            exec(shell_state.args[0], 1);
        } else if (shell_state.input_buffer[0] == 0x1B) { // ESC key
            print_string_colored("Exiting debug mode...", COLOR_LIGHT_RED);
            print_newline();
        }else if (strcmp("sleep", shell_state.command) == 0) {
            if (args_used_amount > 0) {
                int sleep_time;
                str_to_int(shell_state.args[0], &sleep_time);
                if (sleep_time > 0) {
                    syscall(35, (uint32_t) &sleep_time, 0, 0); // Sleep for specified seconds
                    print_string_colored("Slept for ", COLOR_LIGHT_GREEN);
                    print_string_colored(shell_state.args[0], COLOR_LIGHT_GREEN);
                    print_string_colored(" milliseconds.", COLOR_LIGHT_GREEN);
                    print_newline();
                } else {
                    print_string_colored("Invalid sleep time.", COLOR_LIGHT_RED);
                    print_newline();
                }
            } else {
                print_string_colored("Usage: sleep <milliseconds>", COLOR_LIGHT_RED);
                print_newline();
            }
        }else if(strcmp("testmalloc", shell_state.command) == 0) {
            test_malloc_free();
        }else {
            print_string_colored("Command not found: ", COLOR_LIGHT_RED);

            print_string_colored(shell_state.command, COLOR_WHITE);
            print_newline();
            print_string_colored("Type 'help' for available commands.", COLOR_DARK_GRAY);
            print_newline();
        }
    }
}

// syscall wrapper that returns pointer from malloc syscall
void* user_malloc(size_t size) {
    void* ptr = NULL;
    syscall(51, (uint32_t)size, (uint32_t)&ptr, 0);
    return ptr;
}

// syscall wrapper for free
void user_free(void* ptr) {
    syscall(52, (uint32_t)ptr, 0, 0);
}

// Simple test for malloc and free
void test_malloc_free() {
    print_string_at_cursor("Starting malloc/free test...\n");

    // Allocate 64 bytes
    void* p = user_malloc(64);
    if (p == NULL) {
        print_string_at_cursor("malloc failed\n");
        return;
    }
    print_string_at_cursor("malloc succeeded\n");

    // Write some data
    char* data = (char*)p;
    for (int i = 0; i < 64; i++) {
        data[i] = (char)(i + 1);
    }

    // Verify data
    int error = 0;
    for (int i = 0; i < 64; i++) {
        if (data[i] != (char)(i + 1)) {
            error = 1;
            break;
        }
    }
    if (error) {
        print_string_at_cursor("Data verification failed\n");
    } else {
        print_string_at_cursor("Data verification succeeded\n");
    }

    // Free allocated memory
    user_free(p);
    print_string_at_cursor("Memory freed\n");

    print_string_at_cursor("malloc/free test done\n");
}

int main(void) {
    extern void cd_root();
    cd_root();

    cursor.row = 0;
    cursor.col = 0;

    clear_input_buffer();
    print_prompt();

    syscall(7, 0, 0, 0); // activate keyboard

    char c;

    show_cursor();

    while (true) {
        syscall(4, (uint32_t)&c, 0, 0);
        // syscall(14, 0, 0, 0);

        if (c != 0) {
            // Reset completion state on any key except tab
            if (c != '\t') {
                reset_completion_state();
            }
            
            if (c == 0x11) { // Left arrow
                move_cursor_left();
                set_hardware_cursor();
                show_cursor();
                continue;
            } else if (c == 0x12) { // Right arrow
                move_cursor_right();
                set_hardware_cursor();
                show_cursor();
                continue;
            } else if (c == 0x10) { // Up arrow - Navigate to previous command
                handle_up_arrow();
                continue;
            } else if (c == 0x13) { // Down arrow - Navigate to next command
                handle_down_arrow();
                continue;
            } else if (c == '\t') { // Tab key - Cycle through completions
                handle_tab_completion();
                continue;
            }

            if (c == '\n' || c == '\r') { // Enter key
                process_command();
                clear_input_buffer();
                print_prompt();
                show_cursor();
            } else if (c == '\b') { // Backspace
                delete_char_before_cursor();
                redraw_input_line();
            } else if (c >= ' ' && c <= '~') {
                insert_char_at_cursor(c);
                redraw_input_line();
            }
        }
        
        check_and_scroll();
    }

    return 0;
}