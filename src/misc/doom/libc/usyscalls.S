#include "syscall.h"
.intel_syntax noprefix
.section .text

.globl read
.type read, @function
read:
    mov eax, SYSCALL_READ
    int 0x30
    ret

.globl write
.type write, @function
write:
    mov eax, SYSCALL_WRITE
    int 0x30
    ret

.globl open
.type open, @function
open:
    mov eax, SYSCALL_OPEN
    int 0x30
    ret

.globl close
.type close, @function
close:
    mov eax, SYSCALL_CLOSE
    int 0x30
    ret

.globl sbrk
.type sbrk, @function
sbrk:
    mov eax, SYSCALL_SBRK
    int 0x30
    ret

.globl exec
.type exec, @function
exec:
    mov eax, SYSCALL_EXEC
    int 0x30
    ret

.globl exit
.type exit, @function
exit:
    mov eax, SYSCALL_EXIT
    int 0x30
    ret

.globl wait
.type wait, @function
wait:
    mov eax, SYSCALL_WAIT
    int 0x30
    ret

.globl lseek
.type lseek, @function
lseek:
    mov eax, SYSCALL_LSEEK
    int 0x30
    ret

.globl time
.type time, @function
time:
    mov eax, SYSCALL_TIME
    int 0x30
    ret

.globl sleep
.type sleep, @function
sleep:
    mov eax, SYSCALL_SLEEP
    int 0x30
    ret

.globl ioctl
.type ioctl, @function
ioctl:
    mov eax, SYSCALL_IOCTL
    int 0x30
    ret

.globl rename
.type rename, @function
rename:
    mov eax, SYSCALL_RENAME
    int 0x30
    ret

.globl unlink
.type unlink, @function
unlink:
    mov eax, SYSCALL_UNLINK
    int 0x30
    ret

.globl mkdir
.type mkdir, @function
mkdir:
    mov eax, SYSCALL_MKDIR
    int 0x30
    ret

.section .note.GNU-stack
