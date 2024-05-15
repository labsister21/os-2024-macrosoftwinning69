#include "header/cpu/portio.h"

enum {
    cmos_address = 0x70,
    cmos_data    = 0x71
};
 
int get_update_in_progress_flag();
 
unsigned char get_RTC_register(int reg);

uint8_t bcd_to_binary(unsigned char bcd);
 
void read_rtc();

uint8_t get_hour();

uint8_t get_minute();

uint8_t get_second();