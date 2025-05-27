#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : : "r"(eax));
    __asm__ volatile("int $0x30");
}

typedef struct {
    int32_t row;
    int32_t col;
} CP;

struct Time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

// Clock constants
#define CLOCK_ROW 24
#define CLOCK_COL 56
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 25

void format_two_digits(uint8_t num, char* str) {
    str[0] = '0' + (num / 10);
    str[1] = '0' + (num % 10);
}

void print_at_position(const char* str, int row, int col, uint8_t color) {
    CP pos = {row, col};
    int i = 0;
    while (str[i] != '\0') {
        CP current_pos = {pos.row, pos.col + i};
        syscall(5, (uint32_t)&str[i], color, (uint32_t)&current_pos);
        i++;
    }
}

void clear_clock_area() {
    char spaces[] = "        ";
    CP pos = {24, 56}; // CLOCK_ROW = 24, CLOCK_COL = 56
    
    for (int i = 0; i < 8; i++) {
        CP current_pos = {pos.row, pos.col + i};
        char space = ' ';
        syscall(5, (uint32_t)&space, 0x00, (uint32_t)&current_pos);
    }
}

void draw_clock_with_priority(const char* time_str) {
    CP pos = {24, 56}; // CLOCK_ROW = 24, CLOCK_COL = 56
    
    for (int i = 0; i < 8 && time_str[i] != '\0'; i++) {
        CP current_pos = {pos.row, pos.col + i};
        syscall(5, (uint32_t)&time_str[i], 0x0E, (uint32_t)&current_pos);
    }
}

bool is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t days_in_month(uint8_t month, uint16_t year) {
    uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

void convert_to_utc_plus_7(struct Time* time) {
    time->hour += 7;
    
    if (time->hour >= 24) {
        time->hour -= 24;
        time->day += 1;
        
        uint8_t max_days = days_in_month(time->month, time->year);
        if (time->day > max_days) {
            time->day = 1;
            time->month += 1;
            
            if (time->month > 12) {
                time->month = 1;
                time->year += 1;
            }
        }
    }
}

int main(void) {
    struct Time current_time;
    struct Time last_time;
    
    memset(&last_time, 0xFF, sizeof(struct Time));

    syscall(7, 0, 0, 0);
    
    while (true) {
        memset(&current_time, 0x0, sizeof(struct Time));
        
        syscall(34, (uint32_t)&current_time, 0, 0);

        convert_to_utc_plus_7(&current_time);
        
        if (current_time.second != last_time.second ||
            current_time.minute != last_time.minute ||
            current_time.hour != last_time.hour) {
            
            // Format: "HH:MM:SS"
            char time_str[9];
            format_two_digits(current_time.hour, &time_str[0]);
            time_str[2] = ':';
            format_two_digits(current_time.minute, &time_str[3]);
            time_str[5] = ':';
            format_two_digits(current_time.second, &time_str[6]);
            time_str[8] = '\0';
            
            draw_clock_with_priority(time_str);
            
            last_time = current_time;
        }

        char c;
        syscall(4, (uint32_t)&c, 0, 0);
        if (c == 'q' || c == 'Q' || c == '\x1B') {
            clear_clock_area();
            break;
        }
        
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    return 0;
}