ENTRY(_start)

SECTIONS {
    . = 0x00000000; /* Start address */

    .text ALIGN(4): {
        *crt0.o(.text)
        *(.text)
    }

    .data ALIGN(4): {
        *(.data)
    }

    .bss ALIGN(4): {
        *(COMMON)
        *(.bss)
    }

    .rodata ALIGN(4): {
        *(.rodata*)
    }

    _end = .; /* Define _end symbol here */
    _linker_doom_end = .;

    ASSERT((_linker_doom_end <= 8 * 1024 * 1024), "Error: DOOM program linking result is more than 8 MiB")
}
