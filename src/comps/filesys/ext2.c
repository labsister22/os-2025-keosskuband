#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/text/framebuffer.h"
#include "header/filesys/ext2.h"
#include "header/driver/disk.h"
//#include <stdio.h>

int local_row = 0, local_col = 0;
static struct EXT2Superblock superblock;
static struct EXT2BlockGroupDescriptorTable bgdt;

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
// static struct EXT2Superblock superblock;
// static struct EXT2BlockGroupDescriptorTable bgdt;

// void print_directory_entries(uint32_t inode) {
//     struct EXT2Inode node;
//     uint32_t bgd = inode_to_bgd(inode);
//     uint32_t local = inode_to_local(inode);
//     struct EXT2InodeTable table;
//     read_blocks(&table, bgdt.table[bgd].bg_inode_table, 1);
//     node = table.table[local];

//     if (!(node.i_mode & EXT2_S_IFDIR)) {
//         printf("Not a directory\n");
//         return;
//     }

//     uint8_t buf[BLOCK_SIZE];
//     read_blocks(buf, node.i_block[0], 1);

//     printf("Directory entries:\n");
//     uint32_t offset = 0;
//     while (offset < BLOCK_SIZE) {
//         struct EXT2DirectoryEntry *entry = get_directory_entry(buf, offset);
//         if (entry->inode != 0) {
//             printf(" - Inode: %u, Name: ", entry->inode);
//             for (int i = 0; i < entry->name_len; i++) putchar(entry->name[i]);
//             putchar('\n');
//         }
//         offset += entry->rec_len;
//     }
// }


// Helper functions
char *get_entry_name(void *entry) {
    struct EXT2DirectoryEntry *e = (struct EXT2DirectoryEntry *)entry;
    // Allocate a buffer to return a C-string
    static char name[256];
    memset(name, 0, sizeof(name));
    memcpy(name, e->name, e->name_len);  // Not null-terminated in struct
    name[e->name_len] = '\0';            // Manually null-terminate
    return name;
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
    struct EXT2DirectoryEntry *first = (struct EXT2DirectoryEntry *)ptr;
    struct EXT2DirectoryEntry *second = get_next_directory_entry(first);
    return first->rec_len + second->rec_len;
}

struct EXT2Inode load_inode(uint32_t inode_num) {
    struct EXT2InodeTable inode_table;
    struct EXT2Inode result;

    uint32_t bgd_index = inode_to_bgd(inode_num);
    uint32_t offset = (inode_num - 1) / (BLOCK_SIZE / INODE_SIZE);
    uint32_t local_index = (inode_num - 1) % (BLOCK_SIZE / INODE_SIZE);

    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table + offset, 1);
    result = inode_table.table[local_index];

    return result;
}

// Main Filesystem Functions
uint32_t inode_to_bgd(uint32_t inode) {
    // Convert inode number to block group descriptor index
    // Inodes start at 1, so we subtract 1 to make it 0-based
    return (inode - 1) / INODES_PER_GROUP;
}

// Get local index of inode within its block group
uint32_t inode_to_local(uint32_t inode) {
    return (inode - 1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2Inode *node, uint32_t inode, uint32_t parent_inode) {
    uint8_t dir_table[BLOCK_SIZE];
    memset(dir_table, 0, BLOCK_SIZE);

    // "." entry
    struct EXT2DirectoryEntry *entry1 = (struct EXT2DirectoryEntry *)dir_table;
    entry1->inode = inode;
    entry1->file_type = EXT2_FT_DIR;
    entry1->name_len = 1;
    entry1->rec_len = get_entry_record_len(entry1->name_len);
    memcpy(entry1->name, ".", 1);

    // ".." entry
    struct EXT2DirectoryEntry *entry2 = get_next_directory_entry(entry1);
    entry2->inode = parent_inode;
    entry2->file_type = EXT2_FT_DIR;
    entry2->name_len = 2;
    entry2->rec_len = BLOCK_SIZE - entry1->rec_len;
    memcpy(entry2->name, "..", 2);

    // Set inode metadata
    node->i_size = BLOCK_SIZE;
    node->i_mode = EXT2_S_IFDIR;

    // EXT2 expects i_blocks to be in units of 512 bytes
    node->i_blocks = 1;

    // Allocate block and write data
    allocate_node_blocks(dir_table, node, inode_to_bgd(inode));
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, BOOT_SECTOR, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_ext2(void) {
    // Write filesystem signature
    write_blocks(&fs_signature, BOOT_SECTOR, 1);
    
    // Initialize superblock
    memset(&superblock, 0, sizeof(struct EXT2Superblock));
    superblock.s_blocks_count = DISK_SPACE / BLOCK_SIZE;
    superblock.s_inodes_count = GROUPS_COUNT * INODES_PER_GROUP;

    uint32_t reserved_per_group = 1 + 1 + INODES_TABLE_BLOCK_COUNT;
    uint32_t total_reserved = 3 + (reserved_per_group * GROUPS_COUNT);
    superblock.s_free_blocks_count = superblock.s_blocks_count - total_reserved;
    superblock.s_first_data_block = 3 + reserved_per_group; // After group 0's metadata
    
    superblock.s_first_ino = 1; // Root inode
    superblock.s_blocks_per_group = BLOCKS_PER_GROUP;
    superblock.s_frags_per_group = BLOCKS_PER_GROUP;
    superblock.s_inodes_per_group = INODES_PER_GROUP;
    superblock.s_magic = EXT2_SUPER_MAGIC;
    superblock.s_prealloc_blocks = 0;
    superblock.s_prealloc_dir_blocks = 0;
    
    write_blocks(&superblock, 1, 1);
    
    // Initialize Block Group Descriptor Table
    memset(&bgdt, 0, sizeof(struct EXT2BlockGroupDescriptorTable));
    
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {
        uint32_t base_block = (i == 0) ? 3 : (i * BLOCKS_PER_GROUP);
        bgdt.table[i].bg_block_bitmap = base_block;
        bgdt.table[i].bg_inode_bitmap = base_block + 1;
        bgdt.table[i].bg_inode_table = base_block + 2;
        
        if (i == 0) {
            bgdt.table[i].bg_free_blocks_count = BLOCKS_PER_GROUP - 3 - reserved_per_group;
        } else {
            bgdt.table[i].bg_free_blocks_count = BLOCKS_PER_GROUP - reserved_per_group;
        }
        bgdt.table[i].bg_free_inodes_count = INODES_PER_GROUP;
        bgdt.table[i].bg_used_dirs_count = 0;
    }
    
    write_blocks(&bgdt, 2, 1);
    
    // Initialize block bitmaps
    uint8_t block_bitmap[BLOCK_SIZE];
    uint8_t inode_bitmap[BLOCK_SIZE];
    memset(inode_bitmap, 0, BLOCK_SIZE);
    
    for (uint32_t i = 0; i < GROUPS_COUNT; i++) {
        memset(block_bitmap, 0, BLOCK_SIZE);
        uint32_t group_start = i * BLOCKS_PER_GROUP;
        
        if (i == 0) {
            for (int j = 0; j < 3; j++) { // Mark boot/super/BGDT
                block_bitmap[j / 8] |= (1 << (j % 8));
            }
        }
        
        uint32_t base_block = (i == 0) ? 3 : group_start;
        for (int j = 0; j < reserved_per_group; j++) {
            uint32_t block_in_group = (base_block - group_start) + j;
            block_bitmap[block_in_group / 8] |= (1 << (block_in_group % 8));
        }
        
        write_blocks(block_bitmap, bgdt.table[i].bg_block_bitmap, 1);
        write_blocks(inode_bitmap, bgdt.table[i].bg_inode_bitmap, 1);
    }
    
    // Create root directory (inode 1)
    struct EXT2Inode root_inode;
    memset(&root_inode, 0, sizeof(struct EXT2Inode));
    init_directory_table(&root_inode, 1, 1); // Root's parent is itself
    
    // Mark inode 1 as used
    memset(inode_bitmap, 0, BLOCK_SIZE);
    inode_bitmap[0] |= (1 << 0); // Bit 0 corresponds to inode 1
    write_blocks(inode_bitmap, bgdt.table[0].bg_inode_bitmap, 1);
    
    // Update group 0's descriptor
    bgdt.table[0].bg_free_inodes_count = INODES_PER_GROUP - 1;
    bgdt.table[0].bg_used_dirs_count = 1;
    write_blocks(&bgdt, 2, 1);
    
    // Write root inode to inode table
    struct EXT2InodeTable inode_table;
    memset(&inode_table, 0, sizeof(struct EXT2InodeTable));
    inode_table.table[0] = root_inode; // inode 1 is at index 0
    write_blocks(&inode_table, bgdt.table[0].bg_inode_table, 1);
}

void initialize_filesystem_ext2(void) {
    // create_ext2();
    if (is_empty_storage()) {
        create_ext2();
    } else {
        // Read superblock and BGDT
        read_blocks(&superblock, 1, 1);
        read_blocks(&bgdt, 2, 1);
    }
}

bool is_directory_empty(uint32_t inode) {
    struct EXT2Inode node = load_inode(inode);

    // Check if it's a directory
    if ((node.i_mode & EXT2_S_IFDIR) == 0) {
        return false;
    }

    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, node.i_block[0], 1); // Assumes only one block

    uint32_t offset = 0;
    while (offset < BLOCK_SIZE) {
        struct EXT2DirectoryEntry* entry = (struct EXT2DirectoryEntry*)(dir_data + offset);

        // Safety check for corrupted directory
        if (entry->rec_len == 0) break;

        // Check if it's not "." or ".."
        if (entry->inode != 0) {
            if (!(entry->name_len == 1 && entry->name[0] == '.') &&
                !(entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.')) {
                return false;
            }
        }

        offset += entry->rec_len;
    }

    return true; 
}

// CRUD Operations

int8_t read_directory(struct EXT2DriverRequest *request) {
    struct EXT2Inode parent_node = load_inode(request->parent_inode);

    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 3; // Parent folder invalid
    }

    // Read directory block
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);

    // Search for target entry
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    bool found = false;

    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 &&
            entry->name_len == request->name_len &&
            memcmp(entry->name, request->name, request->name_len) == 0) {
            found = true;
            break;
        }
        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) {
            entry = get_directory_entry(dir_data, offset);
        }
    }

    if (!found) return 2; // Not found


    struct EXT2Inode target_node = load_inode(entry->inode);
    if ((target_node.i_mode & EXT2_S_IFDIR) == 0) {
        return 1; // Not a folder
    }

    if (request->buffer_size < BLOCK_SIZE) return 2; // Not enough buffer

    read_blocks(request->buf, target_node.i_block[0], 1);

    return 0; // Success
}

int8_t read(struct EXT2DriverRequest request) {
    struct EXT2Inode parent_node = load_inode(request.parent_inode);

    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) return 4;

    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);

    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    bool found = false;

    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 &&
            entry->name_len == request.name_len &&
            memcmp(entry->name, request.name, request.name_len) == 0) {
            found = true;
            break;
        }
        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) entry = get_directory_entry(dir_data, offset);
    }

    if (!found) return 3;

    // Load file inode
    struct EXT2Inode target_node = load_inode(entry->inode);

    if ((target_node.i_mode & EXT2_S_IFREG) == 0) return 1;

    if (request.buffer_size < target_node.i_size) return 2;

    uint32_t blocks_to_read = (target_node.i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint8_t *buf = (uint8_t *)request.buf;

    bool is_indirect_block_already_readed = false;
    uint32_t indirect_block[BLOCK_SIZE / 4] = {0};

    bool is_double_indirect_loaded = false;
    uint32_t level1_block[BLOCK_SIZE / 4] = {0};
    uint32_t level2_block[BLOCK_SIZE / 4];


    for (uint32_t i = 0; i < blocks_to_read; i++) {
        if (i == 775) {
            int a = 10; // Debug breakpoint
        }

        if (i < 12) {
            if (target_node.i_block[i] == 0) break;
            read_blocks(buf + (i * BLOCK_SIZE), target_node.i_block[i], 1);
        } else if (i < 140) {
            if (target_node.i_block[12] == 0) return 3; // no indirect block

            
            if (!is_indirect_block_already_readed) {
                is_indirect_block_already_readed = true;
                read_blocks(indirect_block, target_node.i_block[12], 1);
            }

            if (indirect_block[i - 12] == 0) break;
            read_blocks(buf + (i * BLOCK_SIZE), indirect_block[i - 12], 1);
        } else {
            if (target_node.i_block[13] == 0) return 3; 

            if (!is_double_indirect_loaded) {
                is_double_indirect_loaded = true;
                read_blocks(level1_block, target_node.i_block[13], 1);
            }

            uint32_t relative_index = i - 140;
            uint32_t level1_index = relative_index / 127;
            uint32_t level2_index = relative_index % 127;

            if (level1_block[level1_index] == 0) break;

            read_blocks(level2_block, level1_block[level1_index], 1);

            if (level2_block[level2_index] == 0) break;

            read_blocks(buf + (i * BLOCK_SIZE), level2_block[level2_index + 1], 1);
        }
    }


    return 0;
}

uint32_t allocate_node(void) {
    for (uint32_t g = 0; g < GROUPS_COUNT; g++) {
        if (bgdt.table[g].bg_free_inodes_count > 0) {
            uint8_t bitmap[BLOCK_SIZE];
            read_blocks(bitmap, bgdt.table[g].bg_inode_bitmap, 1);

            for (uint32_t i = 0; i < INODES_PER_GROUP; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx = i % 8;

                if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
                    // Mark as used
                    bitmap[byte_idx] |= (1 << bit_idx);
                    write_blocks(bitmap, bgdt.table[g].bg_inode_bitmap, 1);

                    // Update group descriptor
                    bgdt.table[g].bg_free_inodes_count--;
                    write_blocks(&bgdt, 2, 1);

                    // Update superblock
                    superblock.s_free_inodes_count--;
                    write_blocks(&superblock, 1, 1);

                    return g * INODES_PER_GROUP + i + 1; // 1-based inode
                }
            }
        }
    }

    return 0; // No free inode
}

void allocate_node_blocks(void *ptr, struct EXT2Inode *node, uint32_t prefered_bgd) {
    uint32_t blocks_needed = node->i_blocks;
    uint32_t blocks_allocated = 0;
    uint8_t *data = (uint8_t*)ptr;

    // Clear direct blocks
    for (int i = 0; i < 15; ++i) node->i_block[i] = 0;


    uint32_t indirect_block_entries[BLOCK_SIZE / 4]; 
    memset(indirect_block_entries, 0, sizeof(indirect_block_entries));
    uint32_t indirect_count = 0;
    bool indirect_block_allocated = false;

    uint32_t double_indirect_level1[BLOCK_SIZE / 4] = {0};
    uint32_t double_indirect_level2[BLOCK_SIZE / 4] = {0};
    uint32_t double_level1_count = 0;
    uint32_t double_level2_count = 0;
    bool double_indirect_block_allocated = false;

    // Search from preferred group onward
    for (uint32_t g = prefered_bgd; g < GROUPS_COUNT && blocks_allocated < blocks_needed; g++) {
        if (bgdt.table[g].bg_free_blocks_count == 0) continue;

        uint8_t bitmap[BLOCK_SIZE];
        read_blocks(bitmap, bgdt.table[g].bg_block_bitmap, 1);

        uint32_t group_blocks_taken = 0;

        for (uint32_t i = 0; i < BLOCKS_PER_GROUP && blocks_allocated < blocks_needed; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;

            if (blocks_allocated == 790) {
                int a = 10;
            }

            if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
                uint32_t block_id = g * BLOCKS_PER_GROUP + i;

                // Mark block as used
                bitmap[byte_idx] |= (1 << bit_idx);
                group_blocks_taken++;

                // Assign to direct blocks only
                if (blocks_allocated < 12) {
                    node->i_block[blocks_allocated] = block_id;

                    if (data != NULL) {
                        write_blocks(data + (blocks_allocated * BLOCK_SIZE), block_id, 1);
                    }
                } else if (blocks_allocated < 140) {
                    if (!indirect_block_allocated) {
                        node->i_block[12] = block_id;
                        indirect_block_allocated = true;
                        continue;
                    }
                    indirect_block_entries[indirect_count++] = block_id;

                    if (data != NULL) {
                        write_blocks(data + (blocks_allocated * BLOCK_SIZE), block_id, 1);
                    }
                } else {
                    if (!double_indirect_block_allocated) {
                        node->i_block[13] = block_id;
                        double_indirect_block_allocated = true;
                        continue;
                    }
                    if (double_level2_count == 0) {
                        double_indirect_level2[0] = block_id;
                        double_level2_count++;
                        continue;
                    }
                    if (double_level2_count < BLOCK_SIZE / 4) {
                        write_blocks(data + blocks_allocated * BLOCK_SIZE, block_id, 1);
                        double_indirect_level2[double_level2_count++] = block_id;
                    }

                    if (double_level2_count == BLOCK_SIZE / 4 || blocks_allocated + 1 == blocks_needed) {
                        uint32_t level2_block_id = double_indirect_level2[0];
                        write_blocks(&double_indirect_level2, level2_block_id, 1);
                        double_indirect_level1[double_level1_count++] = level2_block_id;
                        double_level2_count = 0;
                    }
                }

                blocks_allocated++;
            }
        }

        // Update block bitmap and BGDT
        write_blocks(bitmap, bgdt.table[g].bg_block_bitmap, 1);
        bgdt.table[g].bg_free_blocks_count -= group_blocks_taken;
        write_blocks(&bgdt, 2, 1);
    }

    if (indirect_block_allocated && indirect_count > 0) {
        write_blocks(indirect_block_entries, node->i_block[12], 1);
    }

    if (double_indirect_block_allocated && double_level1_count > 0) {
        write_blocks(double_indirect_level1, node->i_block[13], 1);
    }

    // Update i_blocks in 512-byte units
    node->i_blocks = blocks_allocated;
}


void sync_node(struct EXT2Inode *node, uint32_t inode) {
    uint32_t bgd_index = inode_to_bgd(inode);
    uint32_t offset = (inode - 1) / (BLOCK_SIZE / INODE_SIZE);
    uint32_t local_index = (inode - 1) % (BLOCK_SIZE / INODE_SIZE);
    
    // Read inode table
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table + offset, 1);
    
    // Update inode
    inode_table.table[local_index] = *node;
    
    // Write back
    write_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table + offset, 1);
}

int8_t write(struct EXT2DriverRequest *request) {
    struct EXT2Inode parent_node = load_inode(request->parent_inode);

    if ((parent_node.i_mode & EXT2_S_IFDIR) == 0) return 2; // Parent not a directory

    // 2. Load parent directory block
    uint8_t dir_data[BLOCK_SIZE];
    read_blocks(dir_data, parent_node.i_block[0], 1);

    // 3. Search for name & track last entry
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(dir_data, offset);
    uint32_t last_entry_offset = 0;
    struct EXT2DirectoryEntry *last_entry = entry;

    while (offset < BLOCK_SIZE) {
        if (entry->inode != 0 &&
            entry->name_len == request->name_len &&
            memcmp(entry->name, request->name, request->name_len) == 0) {
            return 1; // Already exists
        }

        last_entry_offset = offset;
        last_entry = entry;
        offset += entry->rec_len;
        if (offset < BLOCK_SIZE) entry = get_directory_entry(dir_data, offset);
    }

    // 4. Allocate new inode
    uint32_t new_inode = allocate_node();
    if (new_inode == 0) return -1;

    // 5. Prepare new directory entry
    uint16_t new_rec_len = get_entry_record_len(request->name_len);
    uint16_t min_rec_len = get_entry_record_len(last_entry->name_len);
    uint16_t available_space = last_entry->rec_len - min_rec_len;

    struct EXT2DirectoryEntry *new_entry;
    if (available_space >= new_rec_len) {
        last_entry->rec_len = min_rec_len;
        new_entry = get_next_directory_entry(last_entry);
        new_entry->inode = new_inode;
        new_entry->rec_len = available_space;
        new_entry->name_len = request->name_len;
        new_entry->file_type = request->is_directory ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
        memcpy((char *)(new_entry + 1), request->name, request->name_len);
    } else {
        return 5; // Directory block full, no space for entry
    }

    // 6. Initialize new inode
    struct EXT2Inode new_node;
    memset(&new_node, 0, sizeof(struct EXT2Inode));

    char buffer[] = "new inode : ";
    // write_buffer(buffer, 15, local_row, local_col);
    memset(buffer, 0, sizeof(buffer));
    // itoa(new_inode, buffer);
    // write_buffer(buffer, 15, local_row, local_col + 20);
    local_row++;

    if (request->is_directory) {
        init_directory_table(&new_node, new_inode, request->parent_inode);
        int bgd_index = inode_to_bgd(new_inode);
        bgdt.table[bgd_index].bg_used_dirs_count++;
        write_blocks(&bgdt, 2, 1);
    } else {
        new_node.i_mode = EXT2_S_IFREG;
        new_node.i_size = request->buffer_size;
        uint32_t blocks_needed = (request->buffer_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        new_node.i_blocks = blocks_needed;

        allocate_node_blocks(request->buf, &new_node, inode_to_bgd(new_inode));
    }

    // 7. Write new inode and parent
    sync_node(&new_node, new_inode);
    sync_node(&parent_node, request->parent_inode);
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
    uint32_t offset = (inode - 1) / (BLOCK_SIZE / INODE_SIZE);
    uint32_t local_index = (inode - 1) % (BLOCK_SIZE / INODE_SIZE);
    
    // Read inode
    struct EXT2InodeTable inode_table;
    read_blocks(&inode_table, bgdt.table[bgd_index].bg_inode_table + offset, 1);
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

    struct EXT2Inode parent_node = load_inode(request.parent_inode);
    

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
    uint32_t target_inode_num = entry->inode;
    struct EXT2Inode target_inode = load_inode(target_inode_num);

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