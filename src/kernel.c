#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/text/framebuffer.h"
#include "header/kernel-entrypoint.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr); // load gdt

    framebuffer_clear(); //framebuffer
    framebuffer_write(3, 8,  'H', 0, 0xF);
    framebuffer_write(3, 9,  'a', 0, 0xF);
    framebuffer_write(3, 10, 'i', 0, 0xF);
    framebuffer_write(3, 11, '!', 0, 0xF);
    framebuffer_set_cursor(3, 10);


    uint32_t a;
    uint32_t volatile b = 0x0000BABE;
    __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    while (true) b += 1;
}