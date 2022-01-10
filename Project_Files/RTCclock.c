#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "RTCclock.h"
#include "klaw.h"
#include "i2c.h"
#include "lcd1602.h"
#include "MKL05Z4.h"
#include "frdm_bsp.h"

#define OSC32KCLK 0x00

struct tm data;
extern char keyread;
enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};
extern char display[];
//char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

void init(void) 
{
	// enable RTC clock
	SIM->SCGC6 |= SIM_SCGC6_RTC_MASK;
	// select RTC clock source
	SIM->SOPT1 &= ~SIM_SOPT1_OSC32KSEL_MASK;
			
	// Enable external crystal source if clock source is 32KHz
	SIM->SOPT1 |= SIM_SOPT1_OSC32KSEL(OSC32KCLK);
	//SIM->SOPT1 |= SIM_SOPT1_OSC32KSEL(OSC32KCLK);
	//OSC0->CR |= OSC_CR_ERCLKEN_MASK;
	OSC0->CR |= OSC_CR_EREFSTEN_MASK;
}


void rtc_init(void) 
{
	init();

	// Configure the TSR. default value: 1
	RTC->TSR = 1;

	// Configure Time Compensation Register to calibrate RTC accuracy

	// dissable LRL lock
	RTC->LR &= ~RTC_LR_LRL_MASK;
	// RTC->TCR: RTC_TCR_CIR_MASK,RTC_TCR_CIR(x)=0,RTC_TCR_TCR(x)=0  Default no correction
	RTC->TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);

	// enable TCL lock
	RTC->LR |= RTC_LR_TCL_MASK;
	// enable LRL lock
	RTC->LR |= RTC_LR_LRL_MASK;
	// oscillator Enable
	RTC->CR |= RTC_CR_OSCE_MASK;
			
	// enable counter
	RTC->SR |= RTC_SR_TCE_MASK;	
}


time_t rtc_read(void) 
{
	return RTC->TSR;
}

void rtc_write(time_t t) 
{
	// disable counter
	RTC->SR &= ~RTC_SR_TCE_MASK;
	// write seconds
	RTC->TSR = t;
	// re-enable counter
	RTC->SR |= RTC_SR_TCE_MASK;
}


void time_write(int hour, int min, int sec, int month_day, int month, int year, int week_day)
{
	rtc_write(sec); // write seconds to RTC clock
	data.tm_min = min;
	data.tm_hour = hour;
	data.tm_mday = month_day;
	data.tm_mon = month-1;
	data.tm_year = year-1900;
	data.tm_wday = week_day;
}

void time_read(time_t seconds)
{
	data.tm_sec = seconds;	// save the value (from RTC clock)
	
	/*************** Minutes inrementing ***************/
	if(data.tm_sec == 60) // 60 seconds -> increment minutes
	{
		data.tm_min++;
		data.tm_sec = 0;
		rtc_write(0); //restart RTC
	}
	
	/*************** Hours inrementing ***************/
	if(data.tm_min == 60) // 60 minutes -> increment hours
	{
		data.tm_hour++;
		data.tm_min = 0;
	}
	
	/*************** Day of month and Day of week inrementing ***************/
	if(data.tm_hour == 24) // 24 hours -> increment day of the month
	{
		data.tm_mday++;
		data.tm_wday++;
		if(data.tm_wday == 8) // if Sunday 
			data.tm_wday = 0;
		data.tm_hour = 0;
	}
	
	
	/*************** Month inrementing ***************/
	/*************************************************/
	//		January						// March						//May						//July								//August						//October							//December									
	if(data.tm_mon == 0 || data.tm_mon == 2 || data.tm_mon == 4 || data.tm_mon == 6 || data.tm_mon == 7 || data.tm_mon == 9 || data.tm_mon == 11)
	{
		if(data.tm_mday == 32)		//this months have 31 days
		{
			data.tm_mon++;
			data.tm_mday = 1;
		}
	}					//April							//June							//September						//November
	else if(data.tm_mon == 3 || data.tm_mon == 5 || data.tm_mon == 8 || data.tm_mon == 10)
	{
		if(data.tm_mday == 31) 		//this months have 30 days
		{
			data.tm_mon++;
			data.tm_mday = 1;
		}
	}
	else // February
	{
		if(data.tm_year%4 == 0) // If leap year, do napisania jeszcza opcja kiedy rok wypada na stuleciu
		{
			if(data.tm_mday == 30)	// If leap year February has 29 days
			{
				data.tm_mon++;
				data.tm_mday = 1;
			}
		}
		else // If not leap year
		{
			if(data.tm_mday == 29) // If not leap year February has 28 days
			{
				data.tm_mon++;
				data.tm_mday = 1;
			}
		}
	}
	
	
	/*************** Year inrementing ***************/
	if(data.tm_mon == 12)
	{
		data.tm_year++;
		data.tm_mon = 0;
	}
	
}


int set_hour(void)
{
	int hour = 0;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									hour += 1;
									sprintf(display,"%02d",hour);
									LCD1602_SetCursor(5,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									hour -=1;
									sprintf(display,"%02d",hour);
									LCD1602_SetCursor(5,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return hour;
}

int set_minute(void)
{
	int min = 0;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									min += 1;
									sprintf(display,"%02d",min);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									min -=1;
									sprintf(display,"%02d",min);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return min;
}


int set_second(void)
{
	int sec = 0;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									sec += 1;
									sprintf(display,"%02d",sec);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									sec -=1;
									sprintf(display,"%02d",sec);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return sec;
}

int set_monthday(void)
{
	int mon_d = 1;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									mon_d += 1;
									sprintf(display,"%02d",mon_d);
									LCD1602_SetCursor(6,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									mon_d -=1;
									sprintf(display,"%02d",mon_d);
									LCD1602_SetCursor(6,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return mon_d;
}

int set_month(void)
{
	int mon = 1;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									mon += 1;
									sprintf(display,"%02d",mon);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									mon -=1;
									sprintf(display,"%02d",mon);
									LCD1602_SetCursor(4,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return mon;
}

int set_year(void)
{
	int year = 2021;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									year += 1;
									sprintf(display,"%02d",year);
									LCD1602_SetCursor(5,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									year -=1;
									sprintf(display,"%02d",year);
									LCD1602_SetCursor(5,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return year;
}

int set_weekday(void)
{
	int week_d = 1;
	
	while(1)
							{
								keyread = read_keypad();
								if (keyread==SW1) // ADD
								{
									contact_vibration();
									week_d += 1;
									sprintf(display,"%d",week_d);
									LCD1602_SetCursor(7,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW2) // SUB
								{
									contact_vibration();
									week_d -=1;
									sprintf(display,"%d",week_d);
									LCD1602_SetCursor(7,1);
									LCD1602_Print(display);
								}
								else if(keyread==SW3) // OK
								{
									LCD1602_ClearAll();
									break;
								}
							}
		return week_d;
}

