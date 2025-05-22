#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void write_buffer(char *buf, int size, int row, int col) {
    for (int i = 0; i < size; ++i) {
        framebuffer_write(row, col, buf[i], 0xF, 0);
        ++col;
        if (col >= FRAMEBUFFER_WIDTH) {
            ++row;
            col = 0;
        }
    }
}

void puts(char *buf, int size, int row, int col) {
    for (int i = 0; i < size; ++i) {
        framebuffer_write(row, col, buf[i], 0xF, 0);
        col++;
        if (col >= FRAMEBUFFER_WIDTH) {
            ++row;
            col = 0;
        }
    }
}

void put_char(char *buf, int size, int row, int col) {
    framebuffer_write(row, col, *buf, 0xF, 0);
}

void itoa(int value, char *str) {
    char temp[16];
    int i = 0, is_negative = 0;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    if (is_negative) {
        temp[i++] = '-';
    }

    // Reverse string
    int j = 0;
    while (i--) {
        str[j++] = temp[i];
    }
    str[j] = '\0';
}

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
    // TODO : Implementa

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