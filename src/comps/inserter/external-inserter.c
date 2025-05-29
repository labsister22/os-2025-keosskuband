#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "header/filesys/ext2.h"
#include "header/stdlib/strops.h"
#include "header/driver/disk.h"
#include "header/stdlib/string.h"
#include "misc/ikuyokita.h"
#include "misc/apple.h"

// Global variable
uint8_t *image_storage;
uint8_t *file_buffer;

void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    memcpy(
        (uint8_t*) ptr, 
        image_storage + BLOCK_SIZE*(logical_block_address), 
        block_count*BLOCK_SIZE
    );
}

void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    for (int i = 0; i < block_count; i++) {
        memcpy(
            image_storage + BLOCK_SIZE*(logical_block_address+i), 
            (uint8_t*) ptr + BLOCK_SIZE*i, 
            BLOCK_SIZE
        );
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "inserter: ./inserter <file to insert> <parent cluster index> <storage>\n");
        exit(1);
    }

    // Read storage into memory, requiring 32 MB memory
    image_storage = malloc(32*1024*1024);
    file_buffer   = malloc(32*1024*1024);
    FILE *fptr    = fopen(argv[3], "r");
    fread(image_storage, 32*1024*1024, 1, fptr);
    fclose(fptr);

    // Read target file, assuming file is less than 32 MiB
    FILE *fptr_target = fopen(argv[1], "r");
    size_t filesize   = 0;
    if (fptr_target == NULL)
        filesize = 0;
    else {
        fread(file_buffer, 32*1024*1024, 1, fptr_target);
        fseek(fptr_target, 0, SEEK_END);
        filesize = ftell(fptr_target);
        fclose(fptr_target);
    }

    printf("Filename : %s\n",  argv[1]);
    printf("Filesize : %ld bytes\n", filesize);

    // EXT2 operations
    initialize_filesystem_ext2();
    char *name = argv[1];
    struct EXT2DriverRequest request;
    struct EXT2DriverRequest reqread;
    printf("Filename       : %s\n", name);
    uint8_t *read_buffer = malloc(32*1024*1024);


    /* always try to make a directory first */
    request.buf = NULL; 
    request.name = "misc";
    request.parent_inode = 1;
    request.buffer_size = 0;
    request.name_len = 4;
    request.is_directory = true;
    write(&request);

    request.buf = file_buffer;
    request.buffer_size = filesize;
    request.name = name;
    request.name_len = strlen(name);
    request.is_directory = false;
    sscanf(argv[2], "%u", &request.parent_inode);
    sscanf(argv[1], "%s", request.name);

    reqread = request;
    reqread.buf = read_buffer;
    int retcode = read(reqread);
    if (retcode == 0)
    {
        bool same = true;
        for (uint32_t i = 0; i < filesize; i++)
        {
            if (read_buffer[i] != file_buffer[i])
            {
                printf("not same\n");
                same = false;
                break;
            }
        }
        if (same)
        {
            printf("same\n");
        }
    }

    if (!memcmp(argv[1], "apple", 5)) {
        request.buf = apple_frames;
        request.name = "apple";
        request.parent_inode = 2;
        request.buffer_size = 1095*512;
        request.name_len = 5;
        request.is_directory = false;
    }
    if (!memcmp(argv[1], "ikuyokita", 9) && strlen(argv[1]) == 9) {
        // 44 frames, split into 11 files: 4 frames each (last file has 4 frames)
        int frame_counts[11] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
        int frame_starts[11] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40};
        char *names[11] = {
            "ikuyokita0-3", "ikuyokita4-7", "ikuyokita8-11", "ikuyokita12-15",
            "ikuyokita16-19", "ikuyokita20-23", "ikuyokita24-27", "ikuyokita28-31",
            "ikuyokita32-35", "ikuyokita36-39", "ikuyokita40-43"
        };
        int name_lens[11] = {12, 12, 13, 14, 14, 14, 14, 14, 14, 14, 14};
        for (int i = 0; i < 11; i++) {
            request.buf = ikuyokita_frames[frame_starts[i]];
            request.name = names[i];
            request.parent_inode = 2;
            request.buffer_size = frame_counts[i]*200*512;
            request.name_len = name_lens[i];
            request.is_directory = false;
            retcode = write(&request);
            if (retcode == 0)
                puts("Write success");
            else if (retcode == 1)
                puts("Error: File/folder name already exist");
            else if (retcode == 2)
                puts("Error: Invalid parent node index");
            else
                puts("Error: Unknown error");
        }
        // Write image in memory into original, overwrite them
        fptr = fopen(argv[3], "w");
        fwrite(image_storage, 32 * 1024 * 1024, 1, fptr);
        fclose(fptr);

        return 0;
    }

    bool is_replace = false;
    retcode = write(&request);
    if (retcode == 1 && is_replace)
    {
        retcode = delete (request);
        retcode = write(&request);
    }
    if (retcode == 0)
        puts("Write success");
    else if (retcode == 1)
        puts("Error: File/folder name already exist");
    else if (retcode == 2)
        puts("Error: Invalid parent node index");
    else
        puts("Error: Unknown error");

    // Write image in memory into original, overwrite them
    fptr = fopen(argv[3], "w");
    fwrite(image_storage, 32 * 1024 * 1024, 1, fptr);
    fclose(fptr);

    return 0;
}