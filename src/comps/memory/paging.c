#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/memory/paging.h"
#include "header/stdlib/string.h"

// Aligned at 4KB boundary to prevent triple fault
__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        // Identity mapping for first 4MB (physical 0x0-0x400000 -> virtual 0x0-0x400000)
        // This is needed during boot process before we fully transition to higher half
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
        // Higher half kernel mapping (physical 0x0-0x400000 -> virtual 0xC0000000-0xC0400000)
        [0x300] = { // 0xC0000000 >> 22 = 0x300
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0]                            = true, // First frame (0x0-0x400000) is used by kernel
        [1 ... PAGE_FRAME_MAX_COUNT-1] = false // All other frames are initially free
    },
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT - 1,
    .last_allocated_frame = 0
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    // Extract the upper 10 bits of the virtual address (bits 22-31)
    // These bits are used as an index into the page directory
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    
    // Set the flags in the page directory entry
    page_dir->table[page_index].flag = flag;
    
    // Set the lower address bits (bits 22-31 of physical address)
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    
    // Flush TLB for this virtual address to ensure the change takes effect
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    // The invlpg instruction invalidates a single TLB entry for the given virtual address
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}

/* --- Memory Management --- */
bool paging_allocate_check(uint32_t amount) {
    return page_manager_state.free_page_frame_count >= amount;
}

bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    // Validate virtual address is in user space (below 0xC0000000)
    if ((uint32_t)virtual_addr >= KERNEL_VIRTUAL_BASE) {
        return false; // User pages must be below the kernel space
    }
    
    // Ensure the address is 4MB aligned
    if (((uint32_t)virtual_addr & 0x3FFFFF) != 0) {
        return false; // Address must be 4MB aligned
    }
    
    // Check if we have any free frames
    if (page_manager_state.free_page_frame_count == 0) {
        return false;
    }
    
    // Find a free physical frame using next-fit strategy
    uint32_t frame;
    uint32_t start_frame = (page_manager_state.last_allocated_frame + 1) % PAGE_FRAME_MAX_COUNT;
    uint32_t current_frame = start_frame;
    bool found = false;
    
    do {
        // Skip frame 0 (reserved for kernel)
        if (current_frame > 0 && !page_manager_state.page_frame_map[current_frame]) {
            frame = current_frame;
            found = true;
            break;
        }
        current_frame = (current_frame + 1) % PAGE_FRAME_MAX_COUNT;
    } while (current_frame != start_frame);
    
    // If we can't find a free frame
    if (!found) {
        return false;
    }
    
    // Mark the frame as used
    page_manager_state.page_frame_map[frame] = true;
    page_manager_state.free_page_frame_count--;
    page_manager_state.last_allocated_frame = frame;
    
    // Create page directory entry flag for user page
    struct PageDirectoryEntryFlag flag = {
        .present_bit = 1,         // Page is present in memory
        .write_bit = 1,           // Allow write access
        .user_bit = 1,            // Allow user-mode access
        .use_pagesize_4_mb = 1    // Use 4MB page size
    };
    
    // Calculate physical address from frame number
    void *physical_addr = (void *)(frame * PAGE_FRAME_SIZE);
    
    // Update page directory entry
    update_page_directory_entry(page_dir, physical_addr, virtual_addr, flag);
    
    return true;
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    // Validate virtual address is in user space (below 0xC0000000)
    if ((uint32_t)virtual_addr >= KERNEL_VIRTUAL_BASE) {
        return false; // Cannot free kernel pages through this function
    }
    
    // Get page directory entry index
    uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;
    
    // Check if the page is present
    if (!page_dir->table[page_index].flag.present_bit) {
        return false; // Page not present
    }
    
    // Check if it's a user page (should have user_bit set)
    if (!page_dir->table[page_index].flag.user_bit) {
        return false; // Not a user page
    }
    
    // Get physical frame number from the page directory entry
    uint32_t physical_frame = page_dir->table[page_index].lower_address;
    
    // Mark the frame as free if it's valid
    if (physical_frame < PAGE_FRAME_MAX_COUNT && page_manager_state.page_frame_map[physical_frame]) {
        page_manager_state.page_frame_map[physical_frame] = false;
        page_manager_state.free_page_frame_count++;
    } else {
        // The frame was not allocated or already free
        return false;
    }
    
    // Clear the page directory entry
    memset(&page_dir->table[page_index], 0, sizeof(struct PageDirectoryEntry));
    
    // Flush the TLB for this virtual address
    flush_single_tlb(virtual_addr);
    
    return true;
}