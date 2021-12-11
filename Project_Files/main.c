/*-------------------------------------------------------------------------------------
	Projekt: Alarm
					
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "uart0.h"
#include "klaw.h"
#include "i2c.h"
#include "lcd1602.h"
#include "RTCclock.h"

#define CRET	0xd		// Carriage return

char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
char tx_buf[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

char keyread = 0;
char password[] = {1,2,3,4};

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};


int main (void)
{
	UART0_Init();		// Initialization port UART0
	Klaw_Init();
	init(); 
	rtc_init();
	
	LCD1602_Init();
	LCD1602_Backlight(TRUE);
	
	LCD1602_Print("ALARM UNARMED!");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S1 TO CONTINUE");
	
	while(PTA->PDIR&C1_MASK);		// wait to press SW1
	contact_vibration();		// reduce contact vibration
	LCD1602_ClearAll();
	
	char check;
	
	while(1)
	{
		LCD1602_SetCursor(0,0);
		LCD1602_Print("S1:ADMIN ");
		LCD1602_Print("S2:USER");
		LCD1602_SetCursor(0,1);
		LCD1602_Print("S3:BLUET ");
		LCD1602_Print("S4:CLK");
		keyread = read_keypad();			// reading from 4x4 tact switches and write to keyread
		contact_vibration();
		
		switch(keyread)
		{
			case SW1:
				check = enter_passwd();
				if (check==0)	//wrong password
				{
					for(char chances = 1; chances != 3; chances++)		// you have 3 chances to put correct password, after this time start alarm
					{
						LCD1602_ClearAll();
						LCD1602_Print("WRONG PASSWD");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("TRY AGAIN S16:EX");
						DELAY(1000)
						check = enter_passwd();
						if(chances==2 && check == 0)	//wrong password after 3 chances start alarming
						{
							alarm();		//go to function of alarm
							break;
						}
						else if (check == 1) //good password then go to the admin menu
						{
							admin_setup();
							break;
						}		
					}		
				}
				else if (check == 1) //good password then go to the admin menu
				{
					admin_setup();
					break;
				}	
				

				case SW2:	
					LCD1602_ClearAll();
					LCD1602_Print("TO DO...");
					// TO DO....
				
				
				case SW3:	
					LCD1602_ClearAll();
					LCD1602_Print("TO DO...");
					// TO DO....
				
				
				case SW4:
					time_write(45, 01, 15, 11, 12, 2021, 7); // write to data structure of time 
					LCD1602_ClearAll();
					
					while(1)
					{
						time_read(rtc_read());
			
						sprintf(display,"%s%c",asctime(&data),CRET);
			
						for(int i=0;display[i]!=0;i++)
						{
							while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik jest pusty?
							UART0->D = display[i];		// Wy?lij aktualn? warto?? licznika
						}
						
						sprintf(display,"%d %d %d",data.tm_wday,data.tm_mon,data.tm_mday);
						LCD1602_SetCursor(0,0);
						LCD1602_Print(display);
						sprintf(display,"%02d:%02d:%02d",data.tm_hour,data.tm_min,data.tm_sec);
						LCD1602_SetCursor(0,1);
						LCD1602_Print(display);
						
						keyread = read_keypad();
						if (keyread==SW16) 		// Cancelling the alarm 
						{
							//contact_vibration();
							LCD1602_ClearAll();
							break;
						}
					}
					// TO DO....
					
		}
	
		/*
		if(keyread = read_keypad())
		{ 
			sprintf(tx_buf,"SWITCH=%02d%c",keyread,CRET);
			LCD1602_SetCursor(0,0);
			sprintf(display,"SWITCH=%02d",keyread);
			LCD1602_Print(display);			
			for(i=0;tx_buf[i]!=0;i++)
			{
				while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik jest pusty?
				UART0->D = tx_buf[i];		// Wy?lij aktualn? warto?? licznika
			}
		}
		*/
	}
}
