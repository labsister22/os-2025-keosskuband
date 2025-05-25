global _start
extern main

section .text
_start:
    call main
    ; for now we dont have any syscall for exit
    ; mov ebx, eax 
	; mov eax, 10   
	; int 0x30
    jmp $