#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/filesys/ext2.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '5', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

// Global variables
static struct EXT2Superblock superblock;
static struct EXT2BlockGroupDescriptorTable bgdt;

// Helper functions
char *get_entry_name(void *entry) {
    struct EXT2DirectoryEntry *dir_entry = (struct EXT2DirectoryEntry *)entry;
    return (char *)entry + sizeof(struct EXT2DirectoryEntry);
}

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset) {
    return (struct EXT2DirectoryEntry *)((uint8_t *)ptr + offset);
}

struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry) {
    return (struct EXT2DirectoryEntry *)((uint8_t *)entry + entry->rec_len);
}

uint16_t get_entry_record_len(uint8_t name_len) {
    // Align to 4 bytes boundaries
    return ((sizeof(struct EXT2DirectoryEntry) + name_len + 3) / 4) * 4;
}

uint32_t get_dir_first_child_offset(void *ptr) {
    return 0; // First entry is at the beginning of the directory data
}

// Main Filesystem Functions

uint32_t inode_to_bgd(uint32_t inode) {
    // Convert inode number to block group descriptor index
    // Inodes start at 1, so we subtract 1 to make it 0-based
    return (inode - 1) / INODES_PER_GROUP;
}

uint32_t inode_to_local(uint32_t inode) {
    // Get local index of inode within its block group
    return (inode - 1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2Inode *node, uint32_t inode, uint32_t parent_inode) {
    uint8_t dir_table[BLOCK_SIZE];
    memset(dir_table, 0, BLOCK_SIZE);
    
    // Initialize "." entry (self reference)
    struct EXT2DirectoryEntry *self_entry = (struct EXT2DirectoryEntry *)dir_table;
    self_entry->inode = inode;
    self_entry->file_type = EXT2_FT_DIR;
    self_entry->name_len = 1;
    char *self_name = get_entry_name(self_entry);
    self_name[0] = '.';
    self_entry->rec_len = get_entry_record_len(self_entry->name_len);
    
    // Initialize ".." entry (parent reference)
    struct EXT2DirectoryEntry *parent_entry = get_next_directory_entry(self_entry);
    parent_entry->inode = parent_inode;
    parent_entry->file_type = EXT2_FT_DIR;
    parent_entry->name_len = 2;
    char *parent_name = get_entry_name(parent_entry);
    parent_name[0] = '.';
    parent_name[1] = '.';
    parent_entry->rec_len = BLOCK_SIZE - self_entry->rec_len; // Remaining space
    
    // Allocate one block for directory table
    node->i_blocks = 1;
    node->i_size = BLOCK_SIZE;
    node->i_mode = EXT2_S_IFDIR;
    
    // Write directory table to disk
    allocate_node_blocks(dir_table, node, inode_to_bgd(inode));
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, BOOT_SECTOR, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_ext2(void) {
    // Write filesystem signature
    write_blocks(fs_signature, BOOT_SECTOR, 1);
    
    // Initialize superblock
    memset(&superblock, 0, sizeof(struct EXT2Superblock));
    superblock.s_blocks_count = DISK_SPACE / BLOCK_SIZE;
    superblock.s_inodes_count = GROUPS_COUNT * INODES_PER_GROUP;
    superblock.s_free_blocks_count = superblock.s_blocks_count - 3; // Boot, Superblock, BGDT
    superblock.s_free_inodes_count = superblock.s_inodes_count - 1; // Root inode
    superblock.s_first_data_block = 1; // Superblock is at block 1
    superblock.s_first_ino = 1; // Root inode
    superblock.s_blocks_per_group = BLOCKS_PER_GROUP;
    superblock.s_frags_per_group = BLOCKS_PER_GROUP; // No fragments, same as blocks
    superblock.s_inodes_per_group = INODES_PER_GROUP;
    superblock.s_magic = EXT2_SUPER_MAGIC;
    superblock.s_prealloc_blocks = 0; // No preallocation
    superblock.s_prealloc_dir_blocks = 0; // No preallocation
    
    // Write superblock
    write_blocks(&superblock, 1, 1);
    
    // Initialize Block Group Descriptor Table
    memset(&bgdt, 0, sizeof(struct EXT2BlockGroupDescriptorTable));
    
    // Set up each block group
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {
        uint32_t block_offset = i * BLOCKS_PER_GROUP;
        uint32_t base_block = 3 + (i * 3); // 3 blocks reserved per group: bitmap, inode bitmap, inode table
        
        bgdt.table[i].bg_block_bitmap = base_block;
        bgdt.table[i].bg_inode_bitmap = base_block + 1;
        bgdt.table[i].bg_inode_table = base_block + 2;
        bgdt.table[i].bg_free_blocks_count = BLOCKS_PER_GROUP;
        bgdt.table[i].bg_free_inodes_count = INODES_PER_GROUP;
        bgdt.table[i].bg_used_dirs_count = 0;
    }
    
    // Write BGDT
    write_blocks(&bgdt, 2, 1);
    
    // Initialize block and inode bitmaps for each group
    uint8_t bitmap[BLOCK_SIZE];
    memset(bitmap, 0, BLOCK_SIZE);
    
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {
        // Write block bitmap (mark first few blocks as used)
        write_blocks(bitmap, bgdt.table[i].bg_block_bitmap, 1);
        
        // Write inode bitmap
        write_blocks(bitmap, bgdt.table[i].bg_inode_bitmap, 1);
        
        // Initialize empty inode table
        struct EXT2InodeTable inode_table;
        memset(&inode_table, 0, sizeof(struct EXT2InodeTable));
        write_blocks(&inode_table, bgdt.table[i].bg_inode_table, INODES_TABLE_BLOCK_COUNT);
    }
    
    // Create root directory (inode 1)
    struct EXT2Inode root_inode;
    memset(&root_inode, 0, sizeof(struct EXT2Inode));
    
    // Initialize root directory
    init_directory_table(&root_inode, 1, 1); // Root's parent is itself
    
    // Mark root inode as used in first group's bitmap
    uint8_t inode_bitmap[BLOCK_SIZE];
    read_blocks(inode_bitmap, bgdt.table[0].bg_inode_bitmap, 1);
    inode_bitmap[0] |= 1; // Mark first inode as used
    write_blocks(inode_bitmap, bgdt.table[0].bg_inode_bitmap, 1);
    
    // Update first group's descriptor
    bgdt.table[0].bg_free_inodes_count--;
    bgdt.table[0].bg_used_dirs_count++;
    write_blocks(&bgdt, 2, 1);
    
    // Write root inode to inode table
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[0].bg_inode_table, 1);
    inode_table.table[0] = root_inode; // First inode in the table
    write_blocks(&inode_table, bgdt.table[0].bg_inode_table, 1);
}

void initialize_filesystem_ext2(void) {
    if (is_empty_storage()) {
        create_ext2();
    } else {
        // Read superblock and BGDT
        read_blocks(&superblock, 1, 1);
        read_blocks(&bgdt, 2, 1);
    }
}

bool is_directory_empty(uint32_t inode) {
    struct EXT2Inode node;
    uint32_t bgd_index = inode_to_bgd(inode);
    uint32_t local_index = inode_to_local(inode);
    
    // Read inode from inode table
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    node = inode_table.table[local_index];
    
    // Check if it's a directory
    if ((node.i_mode & EXT2_S_IFDIR) == 0) {
        return false;
    }
    
    // Read directory data
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, node.i_block[0], 1);
    
    // Get first and second entries (. and ..)
    struct EXT2DirectoryEntry *entry1 = get_directory_entry(dir_data, 0);
    struct EXT2DirectoryEntry *entry2 = get_next_directory_entry(entry1);
    
    // Check if there's a third entry
    struct EXT2DirectoryEntry *entry3 = get_next_directory_entry(entry2);
    
    // If entry3 is within the block and has a valid inode, directory is not empty
    return (uint8_t*)entry3 >= dir_data + BLOCK_SIZE || entry3->inode == 0;
}

// CRUD Operations

int8_t read_directory(struct EXT2DriverRequest *request) {
    // Find the directory
    struct EXT2Inode parent_node;
    uint32_t bgd_index = inode_to_bgd(request->parent_inode);
    uint32_t local_index = inode_to_local(request->parent_inode);
    
    // Read parent inode
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    parent_node = inode_table.table[local_index];
    
    // Check if parent is a directory
    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 3; // Parent folder invalid
    }
    
    // Read directory data
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);
    
    // Search for the entry
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    bool found = false;
    
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0) {
            char *name = get_entry_name(entry);
            if (memcmp(name, request->name, request->name_len) == 0 && 
                name[request->name_len] == 0) {
                found = true;
                break;
            }
        }
        
        offset += entry->rec_len;
        if (offset >= BLOCK_SIZE) break;
        entry = get_directory_entry(dir_data, offset);
    }
    
    if (!found) {
        return 2; // Not found
    }
    
    // Check if it's a directory
    struct EXT2Inode target_node;
    bgd_index = inode_to_bgd(entry->inode);
    local_index = inode_to_local(entry->inode);
    
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    target_node = inode_table.table[local_index];
    
    if ((target_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 1; // Not a folder
    }
    
    // Read directory content
    read_blocks(request->buf, target_node.i_block[0], 1);
    
    return 0; // Success
}

int8_t read(struct EXT2DriverRequest request) {
    // Find the file
    struct EXT2Inode parent_node;
    uint32_t bgd_index = inode_to_bgd(request.parent_inode);
    uint32_t local_index = inode_to_local(request.parent_inode);
    
    // Read parent inode
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    parent_node = inode_table.table[local_index];
    
    // Check if parent is a directory
    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 4; // Parent folder invalid
    }
    
    // Read directory data
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);
    
    // Search for the entry
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    bool found = false;
    
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0) {
            char *name = get_entry_name(entry);
            if (memcmp(name, request.name, request.name_len) == 0 && 
                name[request.name_len] == 0) {
                found = true;
                break;
            }
        }
        
        offset += entry->rec_len;
        if (offset >= BLOCK_SIZE) break;
        entry = get_directory_entry(dir_data, offset);
    }
    
    if (!found) {
        return 3; // Not found
    }
    
    // Check if it's a file
    struct EXT2Inode target_node;
    bgd_index = inode_to_bgd(entry->inode);
    local_index = inode_to_local(entry->inode);
    
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    target_node = inode_table.table[local_index];
    
    if ((target_node.i_mode & EXT2_S_IFREG) == 0) {
        return 1; // Not a file
    }
    
    // Check buffer size
    if (request.buffer_size < target_node.i_size) {
        return 2; // Buffer too small
    }
    
    // Read file content
    uint32_t blocks_to_read = (target_node.i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint8_t *buf = (uint8_t*)request.buf;
    
    for (uint32_t i = 0; i < blocks_to_read; i++) {
        if (i < 12) {
            // Direct blocks
            read_blocks(buf + (i * BLOCK_SIZE), target_node.i_block[i], 1);
        } else if (i < 268) {
            // Indirect blocks (13th entry)
            uint32_t indirect_block[BLOCK_SIZE / sizeof(uint32_t)];
            read_blocks(indirect_block, target_node.i_block[12], 1);
            read_blocks(buf + (i * BLOCK_SIZE), indirect_block[i - 12], 1);
        }
        // Support for doubly-indirect blocks could be added here
    }
    
    return 0; // Success
}

uint32_t allocate_node(void) {
    // Find a free inode
    for (uint32_t g = 0; g < GROUPS_COUNT; g++) {
        if (bgdt.table[g].bg_free_inodes_count > 0) {
            // Read inode bitmap
            uint8_t bitmap[BLOCK_SIZE];
            read_blocks(bitmap, bgdt.table[g].bg_inode_bitmap, 1);
            
            // Find a free bit
            for (uint32_t i = 0; i < INODES_PER_GROUP; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx = i % 8;
                
                if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
                    // Mark as used
                    bitmap[byte_idx] |= (1 << bit_idx);
                    write_blocks(bitmap, bgdt.table[g].bg_inode_bitmap, 1);
                    
                    // Update BGDT
                    bgdt.table[g].bg_free_inodes_count--;
                    write_blocks(&bgdt, 2, 1);
                    
                    // Calculate global inode index
                    return g * INODES_PER_GROUP + i + 1;
                }
            }
        }
    }
    
    return 0; // No free inode found
}

void allocate_node_blocks(void *ptr, struct EXT2Inode *node, uint32_t prefered_bgd) {
    uint32_t blocks_needed = node->i_blocks;
    uint32_t blocks_allocated = 0;
    uint8_t *data = (uint8_t*)ptr;
    
    // Find free blocks in each group
    for (uint32_t g = prefered_bgd; g < GROUPS_COUNT && blocks_allocated < blocks_needed; g++) {
        if (bgdt.table[g].bg_free_blocks_count > 0) {
            // Read block bitmap
            uint8_t bitmap[BLOCK_SIZE];
            read_blocks(bitmap, bgdt.table[g].bg_block_bitmap, 1);
            
            // Find free blocks
            for (uint32_t i = 0; i < BLOCKS_PER_GROUP && blocks_allocated < blocks_needed; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx = i % 8;
                
                if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
                    // Calculate global block index
                    uint32_t block_id = g * BLOCKS_PER_GROUP + i;
                    
                    // Mark as used
                    bitmap[byte_idx] |= (1 << bit_idx);
                    
                    // Assign block to inode
                    if (blocks_allocated < 12) {
                        // Direct block
                        node->i_block[blocks_allocated] = block_id;
                        
                        // Write data
                        write_blocks(data + (blocks_allocated * BLOCK_SIZE), block_id, 1);
                    } else {
                        // Indirect block handling would go here
                        // For simplicity, we're only implementing direct blocks
                    }
                    
                    blocks_allocated++;
                }
            }
            
            // Update bitmap
            write_blocks(bitmap, bgdt.table[g].bg_block_bitmap, 1);
            
            // Update BGDT
            uint32_t blocks_taken = (blocks_allocated < bgdt.table[g].bg_free_blocks_count) ? 
                                   blocks_allocated : bgdt.table[g].bg_free_blocks_count;
            bgdt.table[g].bg_free_blocks_count -= blocks_taken;
            write_blocks(&bgdt, 2, 1);
        }
    }
}

void sync_node(struct EXT2Inode *node, uint32_t inode) {
    uint32_t bgd_index = inode_to_bgd(inode);
    uint32_t local_index = inode_to_local(inode);
    
    // Read inode table
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    
    // Update inode
    inode_table.table[local_index] = *node;
    
    // Write back
    write_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
}

int8_t write(struct EXT2DriverRequest *request) {
    // Check if parent inode exists and is a directory
    struct EXT2Inode parent_node;
    uint32_t bgd_index = inode_to_bgd(request->parent_inode);
    uint32_t local_index = inode_to_local(request->parent_inode);
    
    // Read parent inode
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    parent_node = inode_table.table[local_index];
    
    // Check if parent is a directory
    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 2; // Parent folder invalid
    }
    
    // Read directory data
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);
    
    // Check if name already exists
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    uint32_t last_entry_offset = 0;
    struct EXT2DirectoryEntry *last_entry = entry;
    
    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0) {
            char *name = get_entry_name(entry);
            if (memcmp(name, request->name, request->name_len) == 0 && 
                name[request->name_len] == 0) {
                return 1; // Already exists
            }
        }
        
        last_entry_offset = offset;
        last_entry = entry;
        offset += entry->rec_len;
        if (offset >= BLOCK_SIZE) break;
        entry = get_directory_entry(dir_data, offset);
    }
    
    // Allocate new inode
    uint32_t new_inode = allocate_node();
    if (new_inode == 0) {
        return -1; // Unknown error (no free inodes)
    }
    
    // Create new entry in parent directory
    uint16_t rec_len = get_entry_record_len(request->name_len);
    
    // Check if we have enough space in the last entry
    uint16_t min_rec_len = get_entry_record_len(last_entry->name_len);
    uint16_t available_space = last_entry->rec_len - min_rec_len;
    
    struct EXT2DirectoryEntry *new_entry;
    if (available_space >= rec_len) {
        // Split the last entry
        last_entry->rec_len = min_rec_len;
        new_entry = get_next_directory_entry(last_entry);
    } else {
        // No space, would need to allocate a new block for directory
        // For simplicity, we'll just return an error
        return -1;
    }
    
    // Initialize new entry
    new_entry->inode = new_inode;
    new_entry->rec_len = available_space;
    new_entry->name_len = request->name_len;
    new_entry->file_type = request->is_directory ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
    
    // Copy name
    char *name = get_entry_name(new_entry);
    memcpy(name, request->name, request->name_len);
    name[request->name_len] = 0;
    
    // Create inode
    struct EXT2Inode new_node;
    memset(&new_node, 0, sizeof(struct EXT2Inode));
    
    if (request->is_directory) {
        // Create directory
        init_directory_table(&new_node, new_inode, request->parent_inode);
        
        // Update BGD
        bgd_index = inode_to_bgd(new_inode);
        bgdt.table[bgd_index].bg_used_dirs_count++;
        write_blocks(&bgdt, 2, 1);
    } else {
        // Create file
        new_node.i_mode = EXT2_S_IFREG;
        new_node.i_size = request->buffer_size;
        new_node.i_blocks = (request->buffer_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        
        // Allocate blocks and write data
        allocate_node_blocks(request->buf, &new_node, inode_to_bgd(new_inode));
    }
    
    // Write updated inode
    sync_node(&new_node, new_inode);
    
    // Write updated parent directory
    write_blocks(dir_data, parent_node.i_block[0], 1);
    
    return 0; // Success
}

void deallocate_blocks(void *loc, uint32_t blocks) {
    uint32_t *locations = (uint32_t*)loc;
    
    for (uint32_t i = 0; i < blocks && i < 12; i++) {
        if (locations[i] != 0) {
            uint32_t block_id = locations[i];
            uint32_t bgd_index = block_id / BLOCKS_PER_GROUP;
            uint32_t local_block = block_id % BLOCKS_PER_GROUP;
            
            // Update bitmap
            uint8_t bitmap[BLOCK_SIZE];
            read_blocks(bitmap, bgdt.table[bgd_index].bg_block_bitmap, 1);
            
            uint32_t byte_idx = local_block / 8;
            uint32_t bit_idx = local_block % 8;
            bitmap[byte_idx] &= ~(1 << bit_idx);
            
            write_blocks(bitmap, bgdt.table[bgd_index].bg_block_bitmap, 1);
            
            // Update BGDT
            bgdt.table[bgd_index].bg_free_blocks_count++;
            write_blocks(&bgdt, 2, 1);
        }
    }
    
    // Indirect blocks would be handled similarly
}

void deallocate_node(uint32_t inode) {
    uint32_t bgd_index = inode_to_bgd(inode);
    uint32_t local_index = inode_to_local(inode);
    
    // Read inode
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    struct EXT2Inode *node = &inode_table.table[local_index];
    
    // Free blocks
    deallocate_blocks(node->i_block, node->i_blocks);
    
    // Update inode bitmap
    uint8_t bitmap[BLOCK_SIZE];
    read_blocks(bitmap, bgdt.table[bgd_index].bg_inode_bitmap, 1);
    
    uint32_t byte_idx = local_index / 8;
    uint32_t bit_idx = local_index % 8;
    bitmap[byte_idx] &= ~(1 << bit_idx);
    
    write_blocks(bitmap, bgdt.table[bgd_index].bg_inode_bitmap, 1);
    
    // Update BGDT
    bgdt.table[bgd_index].bg_free_inodes_count++;
    if ((node->i_mode & EXT2_S_IFDIR) != 0) {
        bgdt.table[bgd_index].bg_used_dirs_count--;
    }
    write_blocks(&bgdt, 2, 1);
    
    // Clear inode
    memset(node, 0, sizeof(struct EXT2Inode));
    write_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
}

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded) {
    // This function would handle recursive deallocation of blocks
    // For simplicity, we've covered this in deallocate_blocks
    return *last_bgd;
}

int8_t delete(struct EXT2DriverRequest request) {
    // Cegah penghapusan entri spesial "." dan ".."
    if (request.name_len == 1 && memcmp(request.name, ".", 1) == 0) return -1;
    if (request.name_len == 2 && memcmp(request.name, "..", 2) == 0) return -1;

    // Baca inode parent
    struct EXT2Inode parent_node;
    uint32_t bgd_index = inode_to_bgd(request.parent_inode);
    uint32_t local_index = inode_to_local(request.parent_inode);
    
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    parent_node = inode_table.table[local_index];

    // Validasi parent adalah directory
    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) return -1;

    // Baca data direktori parent
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);

    // Cari entry target
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    bool found = false;

    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0) {
            char *name = get_entry_name(entry);
            if (entry->name_len == request.name_len &&
                memcmp(name, request.name, request.name_len) == 0) {
                found = true;
                break;
            }
        }
        offset += entry->rec_len;
        if (offset >= BLOCK_SIZE) break;
        entry = get_next_directory_entry(entry);
    }

    if (!found) return 1;  // Entry tidak ditemukan

    // Validasi folder kosong jika target adalah direktori
    struct EXT2Inode target_inode;
    uint32_t target_inode_num = entry->inode;
    bgd_index = inode_to_bgd(target_inode_num);
    local_index = inode_to_local(target_inode_num);
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table, 1);
    target_inode = inode_table.table[local_index];

    if ((target_inode.i_mode & EXT2_S_IFDIR) != 0) {
        if (!is_directory_empty(target_inode_num)) return 2;  // Folder tidak kosong
    }

    // Deallocasi inode dan blok
    deallocate_node(target_inode_num);

    // Hapus entry di parent directory
    entry->inode = 0;
    write_blocks(dir_data, parent_node.i_block[0], 1);

    return 0;  // Operasi berhasil
}