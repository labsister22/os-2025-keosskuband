#include <stdint.h>
#include <stdbool.h>

// Syscall function (same as your example)
void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : : "r"(eax));
    __asm__ volatile("int $0x30");
}

// Cursor position structure
typedef struct {
    int32_t row;
    int32_t col;
} CP;

// Time structure
struct Time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

// Display position (bottom of screen in graphics mode)
#define CLOCK_ROW 23
#define CLOCK_COL 50

// Simple memset function
void memset_simple(void *ptr, int value, int size) {
    char *p = (char*)ptr;
    for (int i = 0; i < size; i++) {
        p[i] = value;
    }
}

// Convert number to ASCII with leading zero
void format_two_digits(uint8_t num, char* str) {
    str[0] = '0' + (num / 10);
    str[1] = '0' + (num % 10);
}

// Print string at specific position using graphics syscalls
void print_at_position(const char* str, int row, int col, uint8_t color) {
    CP pos = {row, col};
    int i = 0;
    while (str[i] != '\0') {
        CP current_pos = {pos.row, pos.col + i};
        syscall(5, (uint32_t)&str[i], color, (uint32_t)&current_pos);
        i++;
    }
}

// Clear clock area
void clear_clock_area() {
    char spaces[] = "                    "; // 20 spaces
    print_at_position(spaces, CLOCK_ROW, CLOCK_COL, 0x00);
}

int main(void) {
    struct Time current_time;
    struct Time last_time;
    
    // Initialize last_time with invalid values
    memset_simple(&last_time, 0xFF, sizeof(struct Time));
    
    // Activate keyboard for exit detection
    syscall(7, 0, 0, 0);
    
    while (true) {
        // Reset current time
        memset_simple(&current_time, 0x0, sizeof(struct Time));
        
        // Get current time via syscall 34
        syscall(34, (uint32_t)&current_time, 0, 0);
        
        // Only update display if time changed
        if (current_time.second != last_time.second ||
            current_time.minute != last_time.minute ||
            current_time.hour != last_time.hour) {
            
            // Build time string: "HH:MM:SS"
            char time_str[9];
            format_two_digits(current_time.hour, &time_str[0]);
            time_str[2] = ':';
            format_two_digits(current_time.minute, &time_str[3]);
            time_str[5] = ':';
            format_two_digits(current_time.second, &time_str[6]);
            time_str[8] = '\0';
            
            // Clear and display
            clear_clock_area();
            print_at_position(time_str, CLOCK_ROW, CLOCK_COL, 0x0E); // Yellow color
            
            last_time = current_time;
        }
        
        // Check for exit (q, Q, or ESC)
        char c;
        syscall(4, (uint32_t)&c, 0, 0);
        if (c == 'q' || c == 'Q' || c == '\x1B') {
            clear_clock_area(); // Clean up
            break;
        }
        
        // Small delay
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    return 0;
}