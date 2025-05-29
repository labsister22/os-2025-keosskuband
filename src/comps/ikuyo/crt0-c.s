global _start
extern main

section .text
_start:
    call main
    mov eax, 29
    int 0x30
    ;jmp  $