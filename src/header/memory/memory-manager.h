#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define HEAP_START_ADDR     0x10000000  // Start heap at 256MB -> need to adjust
#define HEAP_MAX_SIZE       0x10000000  // Max 256MB heap -> need to adjust 
#define MIN_BLOCK_SIZE      16          // Minimum allocation size
#define HEAP_MAGIC          0xDEADBEEF  // Magic number for corruption detection

// Block
typedef struct block_header {
    uint32_t magic;                    // Magic number for validation
    size_t size;                       // Size of this block (excluding header)
    bool is_free;                      // Whether this block is free
    struct block_header* next;         // Next block in the list
    struct block_header* prev;         // Previous block in the list
} block_header_t;

typedef struct {
    void* heap_start;                  // Start of heap memory
    void* heap_end;                    // End of allocated heap memory
    size_t heap_size;                  // Current heap size
    block_header_t* first_block;       // First block in the linked list
    bool initialized;                  // Whether heap is initialized
} heap_manager_t;

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);
static int heap_init(void);
static int heap_expand(size_t size);
static block_header_t* find_free_block(size_t size);
static block_header_t* split_block(block_header_t* block, size_t size);
static void merge_free_blocks(block_header_t* block);
static bool is_valid_block(block_header_t* block);


#endif 