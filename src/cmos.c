#include "header/clock/cmos.h"

// int century_register = 0x00;                                // Set by ACPI table parsing code if possible
 
// unsigned char second;
// unsigned char minute;
// unsigned char hour;
// unsigned char day;
// unsigned char month;
// unsigned int year;
 
int get_update_in_progress_flag() {
    out(cmos_address, 0x0A);
    return (in(cmos_data) & 0x80);
}
 
unsigned char get_RTC_register(int reg) {
    out(cmos_address, reg);
    return in(cmos_data);
}

uint8_t bcd_to_binary(unsigned char bcd) {
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xF);
}
 
void read_rtc(struct SyscallClockTimeArgs* clock_time) {
    // Initialize variables
    unsigned char* second = &clock_time->second;
    unsigned char* minute = &clock_time->minute;
    unsigned char* hour = &clock_time->hour;
    unsigned char* day = &clock_time->day;
    unsigned char* month = &clock_time->month;
    unsigned char* year = &clock_time->year;
    
    // unsigned char century;
    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    // unsigned char last_century;
    unsigned char registerB;

    // Note: This uses the "read registers until you get the same values twice in a row" technique
    //       to avoid getting dodgy/inconsistent values due to RTC updates

    while (get_update_in_progress_flag());                // Make sure an update isn't in progress
    *second = get_RTC_register(0x00);
    *minute = get_RTC_register(0x02);
    *hour = get_RTC_register(0x04);
    *day = get_RTC_register(0x07);
    *month = get_RTC_register(0x08);
    *year = get_RTC_register(0x09);
    // if(century_register != 0) {
    //     century = get_RTC_register(century_register);
    // }

    do {
        last_second     = *second;
        last_minute     = *minute;
        last_hour       = *hour;
        last_day        = *day;
        last_month      = *month;
        last_year       = *year;
        // last_century = century;

        while (get_update_in_progress_flag());           // Make sure an update isn't in progress
        *second = get_RTC_register(0x00);
        *minute = get_RTC_register(0x02);
        *hour = get_RTC_register(0x04);
        *day = get_RTC_register(0x07);
        *month = get_RTC_register(0x08);
        // *year = get_RTC_register(0x09);
        // if(century_register != 0) {
        //         century = get_RTC_register(century_register);
        // }
    } while( (last_second != *second) || (last_minute != *minute) || (last_hour != *hour) ||
    (last_day != *day) || (last_month != *month) || (last_year != *year)); /* ||
    (last_century != century) );*/

    registerB = get_RTC_register(0x0B);

    // Convert BCD to binary values if necessary
    if (!(registerB & 0x04)) {
        *second = (*second & 0x0F) + ((*second / 16) * 10);
        *minute = (*minute & 0x0F) + ((*minute / 16) * 10);
        *hour = ( (*hour & 0x0F) + (((*hour & 0x70) / 16) * 10) ) | (*hour & 0x80);
        *day = (*day & 0x0F) + ((*day / 16) * 10);
        *month = (*month & 0x0F) + ((*month / 16) * 10);
        *year = (*year & 0x0F) + ((*year / 16) * 10);
        // if(century_register != 0) {
        //         century = (century & 0x0F) + ((century / 16) * 10);
        // }
    }

    // Convert 12 hour clock to 24 hour clock if necessary

    if (!(registerB & 0x02) && (*hour & 0x80)) {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }
}

// uint8_t get_hour() {
//     read_rtc();
//     return (bcd_to_binary(hour) + 12) % 24;
// }

// uint8_t get_minute() {
//     read_rtc();
//     return bcd_to_binary(minute);
// }

// uint8_t get_second() {
//     read_rtc();
//     return bcd_to_binary(second);
// }

// uint8_t get_day() {
//     read_rtc();
//     if (get_hour() < 12) day++;

//     return bcd_to_binary(day);
// }

// uint8_t get_month() {
//     read_rtc();
//     return bcd_to_binary(month);
// }

// uint8_t get_year() {
//     read_rtc();
//     return bcd_to_binary(year) + 6;
// }