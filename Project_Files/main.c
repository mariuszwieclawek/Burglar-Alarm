/*-------------------------------------------------------------------------------------
	Projekt: Alarm
					
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "i2c.h"
#include "lcd1602.h"
#include "uart0.h"
#include "ADC.h"
#include "klaw.h"
#include "RTCclock.h"
#include "alarm.h"
#include "TPM.h"

#define CRET	0xd		// Carriage return

char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

char keyread = 0;
char password[] = {1,2,3,4};

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};


int main (void)
{
	char check; // Check if password is correct
	uint8_t	kal_error; // For converter A/D calibration
	
	UART0_Init();		// Initialization ports
	Klaw_Init();
	init(); 
	rtc_init();
	LCD1602_Init();
	PWM_Init();				// Inicjalizacja licznika TPM0 (PWM „Low-true pulses”)

	LCD1602_Backlight(TRUE);
	
	kal_error=ADC_Init();				// Initialization and calibration A/C converter
	if(kal_error)
	{	
		LCD1602_SetCursor(0,0);
		LCD1602_Print("CALIBRATION");
		LCD1602_SetCursor(0,0);
		LCD1602_Print("ERROR");
		while(1);									// calibration is not correct
	}
	
	ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(3);		// First trigger ADC0 on channel 8 and enable the interrupt
	
	
	// Display on LCD
	LCD1602_Print("ALARM UNARMED!");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S1 TO CONTINUE");
	
		
	while(PTA->PDIR&C1_MASK);		// Wait to press SW1
	contact_vibration();		// Reduce contact vibration
	LCD1602_ClearAll();
	
	while(1)
	{
		LCD1602_SetCursor(0,0);
		LCD1602_Print("S1:ADMIN ");
		LCD1602_Print("S2:USER");
		LCD1602_SetCursor(0,1);
		LCD1602_Print("S3:BLUET ");
		LCD1602_Print("S4:CLK");
		keyread = read_keypad();			// Reading from 4x4 tact switches and write to keyread
		contact_vibration();
		
		switch(keyread)
		{
			case SW1:
				check = enter_passwd();
				if (check==0)	//wrong password
				{
					for(char chances = 1; chances != 3; chances++)		// You have 3 chances to put correct password, after this time start alarm
					{
						LCD1602_ClearAll();
						LCD1602_Print("WRONG PASSWD");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("TRY AGAIN S16:EX");
						DELAY(1000)
						check = enter_passwd();
						if(chances==2 && check == 0)	// Wrong password after 3 chances start alarming
						{
							alarm();		// Go to function of alarm
							break;
						}
						else if (check == 1) // Correct password then go to the admin menu
						{
							admin_setup();
							break;
						}		
					}		
				}
				else if (check == 1) // Correct password then go to the admin menu
				{
					admin_setup();
					break;
				}	
				

			case SW2:	
				LCD1602_ClearAll();
				LCD1602_Print("TO DO...");
				// TO DO....
				
				
			case SW3:	
				check = enter_passwd();
				if (check==0)	//wrong password
				{
					LCD1602_ClearAll();
					LCD1602_Print("WRONG PASSWORD");
					DELAY(1000)
					break;
				}
				else if (check == 1) // Correct password then go to the bluetooth menu
				{
					LCD1602_ClearAll();
					LCD1602_Print("BLUETOOTH");
					LCD1602_SetCursor(0,1);
					LCD1602_Print("S1:ON     S2:OFF");
					while(1)
					{
						keyread = read_keypad();
						if (keyread==SW1) // TURN ON BLUETOOTH MODULE
						{
							LCD1602_ClearAll();
							//TO DO...
							break;
						}
						else if(keyread==SW2) // TURN OFF BLUETOOTH MODULE
						{
							LCD1602_ClearAll();
							// TO DO...
							break;
						}
						else if(keyread==SW16) // Exit from this menu
						{
							LCD1602_ClearAll();
							break;
						}
					}//end while(1)	
					break;
				}	
				
				
			case SW4:
				LCD1602_ClearAll();
				LCD1602_Print("S1:SET CLOCK");
				LCD1602_SetCursor(0,1);
				LCD1602_Print("S2:TIME   S16:EX");
				while(1)
				{
					keyread = read_keypad();
					if (keyread==SW1) // set the time
					{
						int hour=0,min=0,sec=0;
						int mon_d,month,week_d = 1;
						int year = 2021;
						
						LCD1602_ClearAll();
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("HOUR:");
						hour = set_hour();
							
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("MIN:");
						min = set_minute();
							
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("SEC:");
						sec = set_second();
							
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("MON_D:");
						mon_d = set_monthday();
						
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("MON:");
						month = set_month();
							
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("YEAR:");
						year = set_year();
							
						LCD1602_Print("S1:+ S2:-  S3:OK");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("WEEK_D:");
						week_d = set_weekday();
							
						time_write(hour, min, sec, mon_d, month, year, week_d); // Write to data structure of time 
						time_read(rtc_read()); // read actuall time 
						break;
					}
					else if(keyread==SW2) // display time 
					{
						LCD1602_ClearAll();
						while(1)
						{
							time_read(rtc_read()); // read actuall time 
								
							sprintf(display,"%02d.%02d.%04dr",data.tm_mday,data.tm_mon+1,data.tm_year+1900);
							LCD1602_SetCursor(0,0);
							LCD1602_Print(display);
							sprintf(display,"%02d:%02d:%02d",data.tm_hour,data.tm_min,data.tm_sec);
							LCD1602_SetCursor(0,1);
							LCD1602_Print(display);
								
							keyread = read_keypad();
							if (keyread==SW16) 		// Cancelling the alarm 
							{
								LCD1602_ClearAll();
								break;
							}
						}
						break;
					}
					else if(keyread==SW16) // Exit from this menu
					{
						LCD1602_ClearAll();
						break;
					}
				}//end while(1)	
		}//end switch
	}
}
