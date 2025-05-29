#ifndef _CMOS_H
#define _CMOS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

#define CURRENT_YEAR 2025


#define CMOS_ADDRESS_PORT 0x70
#define CMOS_DATA_PORT    0x71
#define CMOS_SECOND       0x00
#define CMOS_MINUTE       0x02
#define CMOS_HOUR         0x04
#define CMOS_DAY          0x07
#define CMOS_MONTH        0x08
#define CMOS_YEAR         0x09
#define CMOS_CENTURY      0x32
#define STATUS_REGISTER_A 0x0A
#define STATUS_REGISTER_B 0x0B

struct CMOS {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t century;
    uint16_t year;
};

int get_update_in_progress_flag();
unsigned char get_RTC_register(int reg);
void read_rtc(struct CMOS*);


#endif