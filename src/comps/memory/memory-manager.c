#include "header/memory/memory-manager.h"
#include "header/memory/paging.h"

static heap_manager_t heap_manager = {0};

// Internal heap expansion using paging system directly
static int heap_expand_internal(size_t size) {
    // Align to 4MB page boundaries
    size_t aligned_size = (size + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
    
    if (heap_manager.heap_size + aligned_size > HEAP_MAX_SIZE) {
        return -1;
    }
    
    // Get current page directory
    struct PageDirectory* current_pd = paging_get_current_page_directory_addr();
    if (!current_pd) {
        return -1;
    }
    
    // Allocate pages for the expanded heap
    void* current_end = heap_manager.heap_end;
    size_t pages_needed = aligned_size / PAGE_FRAME_SIZE;
    
    for (size_t i = 0; i < pages_needed; i++) {
        void* virtual_addr = (void*)((uintptr_t)current_end + (i * PAGE_FRAME_SIZE));
        
        if (!paging_allocate_user_page_frame(current_pd, virtual_addr)) {
            // Rollback allocated pages on failure
            for (size_t j = 0; j < i; j++) {
                void* rollback_addr = (void*)((uintptr_t)current_end + (j * PAGE_FRAME_SIZE));
                paging_free_user_page_frame(current_pd, rollback_addr);
            }
            return -1;
        }
    }
    
    // Success - update heap manager
    void* new_end = (void*)((uintptr_t)heap_manager.heap_end + aligned_size);
    
    // Handle last block expansion
    block_header_t* last_block = heap_manager.first_block;
    while (last_block && last_block->next) {
        last_block = last_block->next;
    }
    
    if (last_block && last_block->is_free) {
        // Expand existing free block
        last_block->size += aligned_size;
    } else {
        // Create new free block
        block_header_t* new_block = (block_header_t*)heap_manager.heap_end;
        new_block->magic = HEAP_MAGIC;
        new_block->size = aligned_size - sizeof(block_header_t);
        new_block->is_free = true;
        new_block->next = NULL;
        new_block->prev = last_block;
        
        if (last_block) {
            last_block->next = new_block;
        }
    }
    
    heap_manager.heap_end = new_end;
    heap_manager.heap_size += aligned_size;
    return 0;
}

static int heap_init(void) {
    if (heap_manager.initialized) {
        return 0;
    }
    
    // Initial heap size - one page frame
    size_t initial_size = PAGE_FRAME_SIZE;
    
    // Get current page directory
    struct PageDirectory* current_pd = paging_get_current_page_directory_addr();
    if (!current_pd) {
        return -1;
    }
    
    // Allocate initial page for heap
    if (!paging_allocate_user_page_frame(current_pd, (void*)HEAP_START_ADDR)) {
        return -1;
    }
    
    heap_manager.heap_start = (void*)HEAP_START_ADDR;
    heap_manager.heap_end = (void*)(HEAP_START_ADDR + initial_size);
    heap_manager.heap_size = initial_size;
    
    heap_manager.first_block = (block_header_t*)heap_manager.heap_start;
    heap_manager.first_block->magic = HEAP_MAGIC;
    heap_manager.first_block->size = initial_size - sizeof(block_header_t);
    heap_manager.first_block->is_free = true;
    heap_manager.first_block->next = NULL;
    heap_manager.first_block->prev = NULL;
    
    heap_manager.initialized = true;
    return 0;
}

static bool is_valid_block(block_header_t* block) {
    if (!block) return false;
    if (block->magic != HEAP_MAGIC) return false;
    if ((uintptr_t)block < (uintptr_t)heap_manager.heap_start) return false;
    if ((uintptr_t)block >= (uintptr_t)heap_manager.heap_end) return false;
    return true;
}

static block_header_t* find_free_block(size_t size) {
    block_header_t* current = heap_manager.first_block;
    
    while (current) {
        if (!is_valid_block(current)) {
            return NULL;
        }
        
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

static block_header_t* split_block(block_header_t* block, size_t size) {
    if (block->size < size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
        return block;
    }
    
    block_header_t* new_block = (block_header_t*)((uint8_t*)block + sizeof(block_header_t) + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - size - sizeof(block_header_t);
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;
    
    block->size = size;
    block->next = new_block;
    
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
    
    return block;
}

static void merge_free_blocks(block_header_t* block) {
    if (!block || !block->is_free) return;
    
    // Merge with next blocks
    while (block->next && block->next->is_free) {
        block_header_t* next_block = block->next;
        block->size += sizeof(block_header_t) + next_block->size;
        block->next = next_block->next;
        
        if (next_block->next) {
            next_block->next->prev = block;
        }
    }
    
    // Merge with previous block
    if (block->prev && block->prev->is_free) {
        block_header_t* prev_block = block->prev;
        prev_block->size += sizeof(block_header_t) + block->size;
        prev_block->next = block->next;
        
        if (block->next) {
            block->next->prev = prev_block;
        }
    }
}

void* malloc(size_t size) {
    if (size == 0) return NULL;
    
    if (!heap_manager.initialized && heap_init() != 0) {
        return NULL;
    }
    
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }
    size = (size + 7) & ~7; // 8-byte alignment
    
    block_header_t* block = find_free_block(size);
    
    if (!block) {
        if (heap_expand_internal(size + sizeof(block_header_t)) != 0) {
            return NULL;
        }
        block = find_free_block(size);
        if (!block) {
            return NULL;
        }
    }
    
    block = split_block(block, size);
    block->is_free = false;
    
    return (void*)((uint8_t*)block + sizeof(block_header_t));
}

void free(void* ptr) {
    if (!ptr) return;
    
    block_header_t* block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    if (!is_valid_block(block) || block->is_free) {
        return;
    }
    
    block->is_free = true;
    merge_free_blocks(block);
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    block_header_t* block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    if (!is_valid_block(block)) {
        return NULL;
    }
    
    if (block->size >= size) {
        return ptr;
    }
    
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy data
    size_t copy_size = (block->size < size) ? block->size : size;
    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (size_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    
    free(ptr);
    return new_ptr;
}

void* calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    if (num != 0 && total_size / num != size) {
        return NULL; // Overflow check
    }
    
    void* ptr = malloc(total_size);
    if (ptr) {
        uint8_t* bytes = (uint8_t*)ptr;
        for (size_t i = 0; i < total_size; i++) {
            bytes[i] = 0;
        }
    }
    
    return ptr;
}

int syscall_heap_expand(void* addr, size_t size) {
    if (addr == NULL || size == 0) {
        return -1; // Invalid parameters
    }
    
    if (!heap_manager.initialized && heap_init() != 0) {
        return -1; // Failed to initialize heap
    }
    
    if ((uintptr_t)addr < (uintptr_t)heap_manager.heap_start || 
        (uintptr_t)addr > (uintptr_t)heap_manager.heap_end) {
        return -1; // Address outside heap bounds
    }
    
    if ((uintptr_t)addr < (uintptr_t)heap_manager.heap_end) {
        return -1; // Cannot expand backwards or within existing heap
    }
    
    size_t aligned_size = (size + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
    
    if (heap_manager.heap_size + aligned_size > HEAP_MAX_SIZE) {
        return -1; // Would exceed maximum heap size
    }
    
    struct PageDirectory* current_pd = paging_get_current_page_directory_addr();
    if (!current_pd) {
        return -1;
    }
    
    size_t pages_needed = aligned_size / PAGE_FRAME_SIZE;
    void* expansion_start = addr;
    
    for (size_t i = 0; i < pages_needed; i++) {
        void* virtual_addr = (void*)((uintptr_t)expansion_start + (i * PAGE_FRAME_SIZE));
        
        if (!paging_allocate_user_page_frame(current_pd, virtual_addr)) {
            for (size_t j = 0; j < i; j++) {
                void* rollback_addr = (void*)((uintptr_t)expansion_start + (j * PAGE_FRAME_SIZE));
                paging_free_user_page_frame(current_pd, rollback_addr);
            }
            return -1; // Page allocation failed
        }
    }
    
    void* new_heap_end = (void*)((uintptr_t)heap_manager.heap_end + aligned_size);
    
    block_header_t* last_block = heap_manager.first_block;
    while (last_block && last_block->next) {
        last_block = last_block->next;
    }
    
    if (last_block && last_block->is_free && 
        ((uintptr_t)last_block + sizeof(block_header_t) + last_block->size) == (uintptr_t)heap_manager.heap_end) {
        last_block->size += aligned_size;
    } else {
        block_header_t* new_block = (block_header_t*)heap_manager.heap_end;
        new_block->magic = HEAP_MAGIC;
        new_block->size = aligned_size - sizeof(block_header_t);
        new_block->is_free = true;
        new_block->next = NULL;
        new_block->prev = last_block;
        
        if (last_block) {
            last_block->next = new_block;
        } else {
            heap_manager.first_block = new_block;
        }
    }
    
    heap_manager.heap_end = new_heap_end;
    heap_manager.heap_size += aligned_size;
    
    return 0; // Success
}