#include <time.h>
#include "MKL05Z4.h"

#ifndef RTCclock_h
#define RTCclock_h

extern struct tm data;

enum week_days_list{Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday};
enum month_list{January=1,February,March,April,May,June,July,August,September,October,November,December};

void RTC_Init(void); /* Initialization RTC module */
time_t RTC_Read(void); /* Read seconds */
void RTC_Write(time_t t); /* Write seconds */
void time_write(int hour, int min, int sec, int month_day, int month, int year, int week_day); /* Write data to time structure*/
void time_read(time_t seconds); /* Read data from time structure*/
int set_hour(void); /* set hour and return a value which user set */
int set_minute(void); /* set minute and return a value which user set */
int set_second(void); /* set second and return a value which user set */
int set_monthday(void); /* set day of month and return a value which user set */
int set_month(void); /* set month and return a value which user set */
int set_year(void); /* set year and return a value which user set */
int set_weekday(void); /* set day of week and return a value which user set */

#endif
