ENTRY(_start)

SECTIONS {
    . = 0x00000000; /* Assuming OS will load this program at virtual address */
    
    .text ALIGN(4): {
        *crt0-c.o(.text)  /* Simplified - just match any crt0-c.o file */
        *(.text)
    }

    .data ALIGN(4): {
        *(.data)
    }

    .bss ALIGN(4): { /* 9 May Edit: Fixes for static storage variables */
        *(COMMON)
        *(.bss)
    }

    .rodata ALIGN(4): {
        *(.rodata*)
    }
    _linker_clock_end = .;
    ASSERT ((_linker_clock_end <= 1 * 1024 * 1024), "Error: User program linking result is more than 1 MiB")
}