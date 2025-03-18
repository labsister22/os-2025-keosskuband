#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    uint16_t position = r * 80 + c;
    
    // high byte
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (position >> 8) & 0xFF);
    
    // low byte
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, position & 0xFF);
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement

    uint16_t offset = (row * 80 + col) * 2;
    FRAMEBUFFER_MEMORY_OFFSET[offset] = c;
    FRAMEBUFFER_MEMORY_OFFSET[offset + 1] = (bg << 4) | (fg & 0x0F);
}

void framebuffer_clear(void) {
    // TODO : Implement
    for (uint16_t i = 0; i < 80 * 25; i++) {
        uint16_t offset = i * 2;
        FRAMEBUFFER_MEMORY_OFFSET[offset] = 0x00;
        FRAMEBUFFER_MEMORY_OFFSET[offset + 1] = 0x07;
    }

    framebuffer_set_cursor(0, 0);
}