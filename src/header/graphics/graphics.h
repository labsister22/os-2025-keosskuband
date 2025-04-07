#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// VGA Settings
#define VGA_WIDTH   320
#define VGA_HEIGHT  200
#define VGA_MEMORY  ((uint8_t*)0xA0000)

// VGA ports    
#define VGA_AC_INDEX      0x3C0
#define VGA_AC_WRITE      0x3C0
#define VGA_AC_READ       0x3C1
#define VGA_MISC_WRITE    0x3C2
#define VGA_SEQ_INDEX     0x3C4
#define VGA_SEQ_DATA      0x3C5
#define VGA_DAC_INDEX_READ   0x3C7
#define VGA_DAC_INDEX_WRITE  0x3C8
#define VGA_DAC_DATA      0x3C9
#define VGA_MISC_READ     0x3CC
#define VGA_GC_INDEX      0x3CE
#define VGA_GC_DATA       0x3CF
#define VGA_CRTC_INDEX    0x3D4
#define VGA_CRTC_DATA     0x3D5
#define VGA_INSTAT_READ   0x3DA

// Common colors in VGA palette
#define COLOR_BLACK       0x00
#define COLOR_BLUE        0x01
#define COLOR_GREEN       0x02
#define COLOR_CYAN        0x03
#define COLOR_RED         0x04
#define COLOR_MAGENTA     0x05
#define COLOR_BROWN       0x06
#define COLOR_LIGHT_GRAY  0x07
#define COLOR_DARK_GRAY   0x08
#define COLOR_LIGHT_BLUE  0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN  0x0B
#define COLOR_LIGHT_RED   0x0C
#define COLOR_PINK        0x0D
#define COLOR_YELLOW      0x0E
#define COLOR_WHITE       0x0F

/**
 * Initialize VGA graphics mode (320x200 with 256 colors).
 * This will switch the display from text mode to graphics mode.
 */
void graphics_initialize(void);

/**
 * Switch back to text mode (80x25).
 */
void graphics_exit(void);

/**
 * Plot a single pixel at (x,y) with specified color.
 * 
 * @param x X coordinate (0 to VGA_WIDTH-1)
 * @param y Y coordinate (0 to VGA_HEIGHT-1)
 * @param color Color value (0-255)
 */
void graphics_pixel(uint16_t x, uint16_t y, uint8_t color);

/**
 * Draw a horizontal line from (x1,y) to (x2,y).
 *
 * @param x1 Starting X coordinate
 * @param x2 Ending X coordinate
 * @param y Y coordinate
 * @param color Color value (0-255)
 */
void graphics_line_horizontal(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color);

/**
 * Draw a vertical line from (x,y1) to (x,y2).
 *
 * @param x X coordinate
 * @param y1 Starting Y coordinate
 * @param y2 Ending Y coordinate
 * @param color Color value (0-255)
 */
void graphics_line_vertical(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color);

/**
 * Draw a line from (x1,y1) to (x2,y2) using Bresenham's algorithm.
 *
 * @param x1 Starting X coordinate
 * @param y1 Starting Y coordinate
 * @param x2 Ending X coordinate
 * @param y2 Ending Y coordinate
 * @param color Color value (0-255)
 */
void graphics_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);

/**
 * Draw a rectangle with top-left corner at (x,y) with specified width, height and color.
 *
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Color value (0-255)
 */
void graphics_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);

/**
 * Draw a filled rectangle with top-left corner at (x,y) with specified width, height and color.
 *
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Color value (0-255)
 */
void graphics_rect_fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);

/**
 * Draw a circle with center at (x,y) with specified radius and color.
 *
 * @param x X coordinate of center
 * @param y Y coordinate of center
 * @param radius Radius of circle
 * @param color Color value (0-255)
 */
void graphics_circle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);

/**
 * Draw a filled circle with center at (x,y) with specified radius and color.
 *
 * @param x X coordinate of center
 * @param y Y coordinate of center
 * @param radius Radius of circle
 * @param color Color value (0-255)
 */
void graphics_circle_fill(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);

/**
 * Clear the screen with a specified color.
 *
 * @param color Color value (0-255)
 */
void graphics_clear(uint8_t color);

/**
 * Draw a single character at the specified position using the 8x8 font.
 * Uses the font lookup table from paste.txt.
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param color Foreground color (0-255)
 * @param bgcolor Background color (0-255)
 */
void graphics_char(uint16_t x, uint16_t y, unsigned char c, uint8_t color, uint8_t bgcolor);

/**
 * Draw a string at the specified position using the 8x8 font.
 * Uses the font lookup table from paste.txt.
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Foreground color (0-255)
 * @param bgcolor Background color (0-255)
 */
void graphics_string(uint16_t x, uint16_t y, const char* str, uint8_t color, uint8_t bgcolor);

#endif