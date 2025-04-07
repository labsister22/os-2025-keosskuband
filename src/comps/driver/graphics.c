#include "header/graphics/graphics.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"
#include "header/graphics/font.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

int abs(int x) {
    return (x < 0) ? -x : x;
}

// Set a VGA register
// static void vga_set_register(uint16_t index_port, uint16_t data_port, uint8_t index, uint8_t value) {
//     out(index_port, index);
//     out(data_port, value);
// }

#define MISC_OUT_REG        0x3C2
#define CRT_ADDR_REG        0x3D4
#define CRT_DATA_REG        0x3D5
#define SEQ_ADDR_REG        0x3C4
#define SEQ_DATA_REG        0x3C5
#define GRAP_ADDR_REG       0x3CE
#define GRAP_DATA_REG       0x3CF
#define ATTR_ADDR_REG       0x3C0
#define ATTR_DATA_REG       0x3C0
#define DAC_WRITE_ADDR      0x3C8
#define DAC_DATA_REG        0x3C9
#define INPUT_STATUS_1      0x3DA

void graphics_initialize(void) {
    //Miscellaneous output register
    out(MISC_OUT_REG, 0x63);
    
    //Disable CRTC protection
    out(CRT_ADDR_REG, 0x11);
    out(CRT_DATA_REG, in(CRT_DATA_REG) & 0x7F);

    //Sequencer registers - first do sequencer reset
    out(SEQ_ADDR_REG, 0x00); out(SEQ_DATA_REG, 0x03);  // Reset
    out(SEQ_ADDR_REG, 0x01); out(SEQ_DATA_REG, 0x01);  // Clocking Mode
    out(SEQ_ADDR_REG, 0x02); out(SEQ_DATA_REG, 0x0F);  // Map Mask
    out(SEQ_ADDR_REG, 0x03); out(SEQ_DATA_REG, 0x00);  // Character Map Select
    out(SEQ_ADDR_REG, 0x04); out(SEQ_DATA_REG, 0x0E);  // Memory Mode
    
    //End sequencer reset
    out(SEQ_ADDR_REG, 0x00); out(SEQ_DATA_REG, 0x03);

    //CRTC registers buat mode 13h (320x200x256) 
    static const uint8_t crtc_regs[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,  // 0x00-0x07
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08-0x0F
        0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,  // 0x10-0x17
        0xFF                                             // 0x18
    };

    for (uint8_t i = 0; i < sizeof(crtc_regs); i++) {
        out(CRT_ADDR_REG, i);
        out(CRT_DATA_REG, crtc_regs[i]);
    }

    // Graphics Controller registers
    out(GRAP_ADDR_REG, 0x00); out(GRAP_DATA_REG, 0x00);  // Set/Reset
    out(GRAP_ADDR_REG, 0x01); out(GRAP_DATA_REG, 0x00);  // Enable Set/Reset
    out(GRAP_ADDR_REG, 0x02); out(GRAP_DATA_REG, 0x00);  // Color Compare
    out(GRAP_ADDR_REG, 0x03); out(GRAP_DATA_REG, 0x00);  // Data Rotate
    out(GRAP_ADDR_REG, 0x04); out(GRAP_DATA_REG, 0x00);  // Read Map Select
    out(GRAP_ADDR_REG, 0x05); out(GRAP_DATA_REG, 0x40);  // Mode
    out(GRAP_ADDR_REG, 0x06); out(GRAP_DATA_REG, 0x05);  // Miscellaneous
    out(GRAP_ADDR_REG, 0x07); out(GRAP_DATA_REG, 0x0F);  // Color Don't Care
    out(GRAP_ADDR_REG, 0x08); out(GRAP_DATA_REG, 0xFF);  // Bit Mask

    // Attribute Controller registers
    in(INPUT_STATUS_1);  // Reset attribute controller flip-flop
    
    // Palette registers (0-15)
    for (uint8_t i = 0; i < 0x10; i++) {
        out(ATTR_ADDR_REG, i);
        out(ATTR_DATA_REG, i);
    }
    
    // Attribute controller mode registers
    out(ATTR_ADDR_REG, 0x10); out(ATTR_DATA_REG, 0x41);  // Mode Control
    out(ATTR_ADDR_REG, 0x11); out(ATTR_DATA_REG, 0x00);  // Overscan Color
    out(ATTR_ADDR_REG, 0x12); out(ATTR_DATA_REG, 0x0F);  // Color Plane Enable
    out(ATTR_ADDR_REG, 0x13); out(ATTR_DATA_REG, 0x00);  // Horizontal Pixel Panning
    out(ATTR_ADDR_REG, 0x14); out(ATTR_DATA_REG, 0x00);  // Color Select

    // Enable video (important!)
    in(INPUT_STATUS_1);  // Reset flip-flop
    out(ATTR_ADDR_REG, 0x20);  // Bit 5 enables video

    // Set up the standard VGA 256-color palette for mode 13h
    // The standard VGA palette uses 6-bits per RGB component (0-63)
    out(DAC_WRITE_ADDR, 0);

    // First 16 colors - EGA compatibility (maling google)
    static const uint8_t standard_palette[256][3] = {
        {0, 0, 0},       // 0: Black
        {0, 0, 42},      // 1: Blue
        {0, 42, 0},      // 2: Green
        {0, 42, 42},     // 3: Cyan
        {42, 0, 0},      // 4: Red
        {42, 0, 42},     // 5: Magenta
        {42, 21, 0},     // 6: Brown
        {42, 42, 42},    // 7: Light Gray
        {21, 21, 21},    // 8: Dark Gray
        {21, 21, 63},    // 9: Light Blue
        {21, 63, 21},    // 10: Light Green
        {21, 63, 63},    // 11: Light Cyan
        {63, 21, 21},    // 12: Light Red
        {63, 21, 63},    // 13: Light Magenta
        {63, 63, 21},    // 14: Yellow
        {63, 63, 63},    // 15: White
    };

    // Load the first 16 colors (EGA compatibility colors)
    for (int i = 0; i < 16; i++) {
        out(DAC_DATA_REG, standard_palette[i][0]);
        out(DAC_DATA_REG, standard_palette[i][1]);
        out(DAC_DATA_REG, standard_palette[i][2]);
    }

    // Colors 16-255: Generate a color cube + grayscale ramp
    // Colors 16-231: 6x6x6 color cube (216 colors)
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                if (r == 0 && g == 0 && b == 0) continue; // Skip black (already set)
                if (r == 5 && g == 5 && b == 5) continue; // Skip white (already set)
                
                uint8_t red = r * 63 / 5;
                uint8_t green = g * 63 / 5;
                uint8_t blue = b * 63 / 5;
                
                out(DAC_DATA_REG, red);
                out(DAC_DATA_REG, green);
                out(DAC_DATA_REG, blue);
            }
        }
    }

    // Colors 232-255: Grayscale ramp (24 shades)
    for (int i = 1; i < 24; i++) {
        uint8_t gray = i * 63 / 23;
        out(DAC_DATA_REG, gray);
        out(DAC_DATA_REG, gray);
        out(DAC_DATA_REG, gray);
    }
}


void graphics_exit(void) {
   //IMPLEMENT
}

void graphics_pixel(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;
    
    // each pixel is one byte
    uint32_t offset = (uint32_t)y * VGA_WIDTH + x;
    VGA_MEMORY[offset] = color;
}

//for testing
void graphics_line_horizontal(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
    // Ensure x1 <= x2
    if (x1 > x2) {
        uint16_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    // Boundary check
    if (y >= VGA_HEIGHT)
        return;
    if (x1 >= VGA_WIDTH)
        return;
    if (x2 >= VGA_WIDTH)
        x2 = VGA_WIDTH - 1;
    
    // Draw horizontal line
    for (uint16_t x = x1; x <= x2; x++) {
        graphics_pixel(x, y, color);
    }
}

void graphics_line_vertical(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color) {
    // Ensure y1 <= y2
    if (y1 > y2) {
        uint16_t temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    // Boundary check
    if (x >= VGA_WIDTH)
        return;
    if (y1 >= VGA_HEIGHT)
        return;
    if (y2 >= VGA_HEIGHT)
        y2 = VGA_HEIGHT - 1;
    
    // Draw vertical line
    for (uint16_t y = y1; y <= y2; y++) {
        graphics_pixel(x, y, color);
    }
}

void graphics_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    // Implementation of Bresenham's line algorithm
    int16_t dx = abs(x2 - x1);
    int16_t dy = -abs(y2 - y1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;
    
    while (1) {
        graphics_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { 
            if (x1 == x2) break;
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2) break;
            err += dx;
            y1 += sy;
        }
    }
}

void graphics_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color) {
    // Draw four lines to form a rectangle
    graphics_line_horizontal(x, x + width - 1, y, color);                  // Top
    graphics_line_horizontal(x, x + width - 1, y + height - 1, color);     // Bottom
    graphics_line_vertical(x, y, y + height - 1, color);                   // Left
    graphics_line_vertical(x + width - 1, y, y + height - 1, color);       // Right
}

void graphics_rect_fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color) {
    // Boundary check
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;
    
    // Adjust width and height if they go out of bounds
    if (x + width > VGA_WIDTH)
        width = VGA_WIDTH - x;
    if (y + height > VGA_HEIGHT)
        height = VGA_HEIGHT - y;
    
    // Draw filled rectangle using horizontal lines
    for (uint16_t j = 0; j < height; j++) {
        graphics_line_horizontal(x, x + width - 1, y + j, color);
    }
}

void graphics_circle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    // Implementation of Midpoint Circle Algorithm
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x0 = 0;
    int16_t y0 = radius;
    
    graphics_pixel(x, y + radius, color);
    graphics_pixel(x, y - radius, color);
    graphics_pixel(x + radius, y, color);
    graphics_pixel(x - radius, y, color);
    
    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;
        
        graphics_pixel(x + x0, y + y0, color);
        graphics_pixel(x - x0, y + y0, color);
        graphics_pixel(x + x0, y - y0, color);
        graphics_pixel(x - x0, y - y0, color);
        graphics_pixel(x + y0, y + x0, color);
        graphics_pixel(x - y0, y + x0, color);
        graphics_pixel(x + y0, y - x0, color);
        graphics_pixel(x - y0, y - x0, color);
    }
}

void graphics_circle_fill(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    // Implementation using horizontal lines between points on the circle
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x0 = 0;
    int16_t y0 = radius;
    
    // Draw horizontal line through the center
    graphics_line_horizontal(x - radius, x + radius, y, color);
    
    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;
        
        // Draw horizontal lines between points on the circle
        graphics_line_horizontal(x - x0, x + x0, y + y0, color);
        graphics_line_horizontal(x - x0, x + x0, y - y0, color);
        graphics_line_horizontal(x - y0, x + y0, y + x0, color);
        graphics_line_horizontal(x - y0, x + y0, y - x0, color);
    }
}

void graphics_clear(uint8_t color) {
    // Fill the entire screen with a specified color
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = color;
    }
}

void graphics_char(uint16_t x, uint16_t y, unsigned char c, uint8_t color, uint8_t bgcolor) {
    // Boundary check
    if (x >= VGA_WIDTH - 8 || y >= VGA_HEIGHT - 8 || c >= 128)
        return;
    
    // Get font data from lookup table
    const uint8_t* char_data = lookup[(uint8_t)c];
    uint8_t size = char_data[0];
    
    // Clear area for character (8x8 pixels)
    for (uint16_t row = 0; row < 8; row++) {
        for (uint16_t col = 0; col < 8; col++) {
            graphics_pixel(x + col, y + row, bgcolor);
        }
    }
    
    // Draw character pixels
    for (uint8_t i = 1; i <= size; i++) {
        uint8_t data = char_data[i];
        uint8_t row = data / 10;
        uint8_t col = data % 10;
        
        if (row < 8 && col < 8) {
            graphics_pixel(x + col, y + row, color);
        }
    }
}

void graphics_string(uint16_t x, uint16_t y, const char* str, uint8_t color, uint8_t bgcolor) {
    uint16_t current_x = x;
    uint16_t current_y = y;
    
    // Draw each character in the string
    for (size_t i = 0; str[i] != '\0'; i++) {
        // Handle newline character
        if (str[i] == '\n') {
            current_x = x;
            current_y += 8;
            continue;
        }
        
        // Draw character
        graphics_char(current_x, current_y, str[i], color, bgcolor);
        
        // Move to next character position
        current_x += 8;
        
        // Wrap to next line if necessary
        if (current_x >= VGA_WIDTH - 8) {
            current_x = x;
            current_y += 8;
        }
    }
}