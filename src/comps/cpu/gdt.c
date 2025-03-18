#include "header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // Null Descriptor - Index 0
            .segment_low = 0,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0,
            .non_system = 0,
            .privilege_level = 0,
            .present = 0,
            .limit_high = 0,
            .available = 0,
            .long_mode = 0,
            .size = 0,
            .granularity = 0,
            .base_high = 0
        },
        {
            // Kernel Code Segment - Index 1
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA,        // Code, not accessed, readable, not conforming
            .non_system = 1,        // Code or Data segment
            .privilege_level = 0,   // Kernel privilege level
            .present = 1,           // Present in memory
            .limit_high = 0xF,      // Upper 4 bits of limit
            .available = 0,
            .long_mode = 0,
            .size = 1,              // 32-bit operand size
            .granularity = 1,       // 4KB granularity
            .base_high = 0
        },
        {
            // Kernel Data Segment - Index 2
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2,        // Data, not accessed, writable, direction up
            .non_system = 1,        // Code or Data segment
            .privilege_level = 0,   // Kernel privilege level
            .present = 1,           // Present in memory
            .limit_high = 0xF,      // Upper 4 bits of limit
            .available = 0,
            .long_mode = 0,
            .size = 1,              // 32-bit operand size
            .granularity = 1,       // 4KB granularity
            .base_high = 0
        }
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