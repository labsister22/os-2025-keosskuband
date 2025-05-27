#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Virtual memory layout for 128MB system
#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000  // 3GB mark
#define USER_SPACE_START           0x00400000   // 4MB (after first page)
#define USER_SPACE_END             0xBFFFFFFF   // Just before kernel
#define HEAP_START_ADDR            0x08000000   // 128MB virtual
#define HEAP_MAX_SIZE              0x20000000   // 512MB virtual max
#define MIN_BLOCK_SIZE             16
#define HEAP_MAGIC                 0xDEADBEEF

typedef struct block_header {
    uint32_t magic;
    size_t size;
    bool is_free;
    struct block_header* next;
    struct block_header* prev;
} block_header_t;

typedef struct {
    void* heap_start;
    void* heap_end;
    size_t heap_size;
    block_header_t* first_block;
    bool initialized;
} heap_manager_t;

// Public interface
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);

// System call interface (implement these in your syscall handler)
int syscall_heap_expand(void* addr, size_t size);

#endif