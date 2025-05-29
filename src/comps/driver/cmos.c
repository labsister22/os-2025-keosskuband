#include "header/driver/cmos.h"


// derived from https://wiki.osdev.org/CMOS, so please refer to it for more information


int century_register = 0x00;                                // Set by ACPI table parsing code if possible

int get_update_in_progress_flag() {
    out(CMOS_ADDRESS_PORT, STATUS_REGISTER_A);
    return (in(CMOS_DATA_PORT) & 0x80);
}

unsigned char get_RTC_register(int reg) {
    out(CMOS_ADDRESS_PORT, reg);
    return in(CMOS_DATA_PORT);
}

void read_rtc(struct CMOS* rtc) {
    uint8_t century;
    uint8_t last_second;
    uint8_t last_minute;
    uint8_t last_hour;
    uint8_t last_day;
    uint8_t last_month;
    uint16_t last_year;
    uint8_t last_century;
    unsigned char registerB;

    // Note: This uses the "read registers until you get the same values twice in a row" technique
    //       to avoid getting dodgy/inconsistent values due to RTC updates

    while (get_update_in_progress_flag());                // Make sure an update isn't in progress
    uint8_t second = get_RTC_register(CMOS_SECOND);
    uint8_t minute = get_RTC_register(CMOS_MINUTE);
    uint8_t hour = get_RTC_register(CMOS_HOUR);
    uint8_t day = get_RTC_register(CMOS_DAY);
    uint8_t month = get_RTC_register(CMOS_MONTH);
    uint16_t year = get_RTC_register(CMOS_YEAR);
    
    if(century_register != 0) {
        century = get_RTC_register(century_register);
    }

    do {
        last_second = second;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;
        last_century = century;

        while (get_update_in_progress_flag());           // Make sure an update isn't in progress
        second = get_RTC_register(CMOS_SECOND);
        minute = get_RTC_register(CMOS_MINUTE);
        hour = get_RTC_register(CMOS_HOUR);
        day = get_RTC_register(CMOS_DAY);
        month = get_RTC_register(CMOS_MONTH);
        year = get_RTC_register(CMOS_YEAR);

        if(century_register != 0) {
            century = get_RTC_register(century_register);
        }

    } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
            (last_day != day) || (last_month != month) || (last_year != year) ||
            (last_century != century) );

    registerB = get_RTC_register(STATUS_REGISTER_B);

    // Convert BCD to binary values if necessary

    if (!(registerB & 0x04)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);

        if(century_register != 0) {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    // Convert 12 hour clock to 24 hour clock if necessary

    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year

    if(century_register != 0) {
        year += century * 100;
    } else {
        year += (CURRENT_YEAR / 100) * 100;
        if(year < CURRENT_YEAR) year += 100;
    }

    // Store the values in the RTC struct
    rtc->second = second;
    rtc->minute = minute;
    rtc->hour = hour;
    rtc->day = day;
    rtc->month = month;
    rtc->year = year;
}