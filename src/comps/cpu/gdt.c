#include "header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {/* Null Descriptor */
            .segment_high   = 0,
            .segment_low    = 0,
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 0,
            .type_bit       = 0,
            .privilege      = 0,
            .valid_bit      = 0,
            .opr_32_bit     = 0,
            .long_mode      = 0,
            .granularity    = 0
        },
        {/* Kernel Code Descriptor */
            .segment_high   = 0xF,
            .segment_low    = 0xFFFF,
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 1,      // 1 for code and data segments
            .type_bit       = 0xA,    // Code, Execute/Read
            .privilege      = 0,      // Ring 0 (Kernel)
            .valid_bit      = 1,      // Present in memory
            .opr_32_bit     = 1,      // 32-bit protected mode
            .long_mode      = 0,
            .granularity    = 1       // Limit in 4 KiB blocks
        },
        {/* Kernel Data Descriptor */
            .segment_high   = 0xF,
            .segment_low    = 0xFFFF,
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 1,      // 1 for code and data segments
            .type_bit       = 0x2,    // Data, Read/Write
            .privilege      = 0,      // Ring 0 (Kernel)
            .valid_bit      = 1,      // Present in memory
            .opr_32_bit     = 1,      // 32-bit protected mode
            .long_mode      = 0,
            .granularity    = 1       // Limit in 4 KiB blocks
        },
        {/* User Code Descriptor */
            .segment_high   = 0xF,
            .segment_low    = 0xFFFF,
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 1,      // 1 for code and data segments
            .type_bit       = 0xA,    // Code, Execute/Read
            .privilege      = 3,      // Ring 3 (User)
            .valid_bit      = 1,      // Present in memory
            .opr_32_bit     = 1,      // 32-bit protected mode
            .long_mode      = 0,
            .granularity    = 1       // Limit in 4 KiB blocks
        },
        {/* User Data Descriptor */
            .segment_high   = 0xF,
            .segment_low    = 0xFFFF,
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 1,      // 1 for code and data segments
            .type_bit       = 0x2,    // Data, Read/Write
            .privilege      = 3,      // Ring 3 (User)
            .valid_bit      = 1,      // Present in memory
            .opr_32_bit     = 1,      // 32-bit protected mode
            .long_mode      = 0,
            .granularity    = 1       // Limit in 4 KiB blocks
        },
        {/* TSS Descriptor */
            .segment_high   = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low    = sizeof(struct TSSEntry),
            .base_high      = 0,
            .base_mid       = 0,
            .base_low       = 0,
            .non_system     = 0,      // S bit
            .type_bit       = 0x9,
            .privilege      = 0,      // DPL
            .valid_bit      = 1,      // P bit
            .opr_32_bit     = 1,      // D/B bit
            .long_mode      = 0,      // L bit
            .granularity    = 0       // G bit
        },
        {0} // Terminate with a zero entry
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */

struct GDTR _gdt_gdtr = {
    .size = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}