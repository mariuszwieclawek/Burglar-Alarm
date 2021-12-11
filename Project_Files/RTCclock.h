#include <time.h>
#include "MKL05Z4.h"

#ifndef RTCclock_h
#define RTCclock_h

extern struct tm data;

void init(void); 
void rtc_init(void);
time_t rtc_read(void);
void time_write(char sec, char min, char hour, char month_day, char month, int year, char week_day);
void time_read(time_t seconds);

#endif
