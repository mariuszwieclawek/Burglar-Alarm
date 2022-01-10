#include <time.h>
#include "MKL05Z4.h"

#ifndef RTCclock_h
#define RTCclock_h

extern struct tm data;

enum week_days_list{Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday};
enum month_list{January=1,February,March,April,May,June,July,August,September,October,November,December};

void init(void); 
void rtc_init(void);
time_t rtc_read(void);
void time_write(int hour, int min, int sec, int month_day, int month, int year, int week_day);
void time_read(time_t seconds);
int set_hour(void);
int set_minute(void);
int set_second(void);
int set_monthday(void);
int set_month(void);
int set_year(void);
int set_weekday(void);

#endif
