/*-------------------------------------------------------------------------------------
	Project: Alarm
	Burglar Alarm based on light intensity sensor. Accelerometer, bluetooth module.				
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "uart0.h"
#include "i2c.h"
#include "lcd1602.h"
#include "ADC.h"
#include "klaw.h"
#include "RTCclock.h"
#include "alarm.h"
#include "TPM.h"

#define CRET	0xd		// Carriage return
#define LF	0xa		// Enter

char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20}; //buffer

char keyread = 0; // read from keypad
char password[] = {1,2,3,4}; //password for admin
char user_password[] = {1,1,1,1}; //password for user

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};

//UART
char Too_Long[]="Too long text";
char Error[]="Wrong text";
char ALARM_ON[]="ALARMON";
char ALARM_OFF[]="ALARMOFF";
char rx_buf[16];
uint8_t rx_buf_pos=0;
char tempo,buf;
uint8_t rx_FULL=0;
uint8_t too_long=0;
char bluetooth_on=0;

// SysTick counter
uint32_t time_count=0;				// The time counter, counting the seconds from the handler
uint8_t second=0;			// Interrupt counter (up to 10)
uint8_t second_OK=0;		// "1" means that the SysTick handler counted 10 interrupts, each 0.1s, which is one second


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////SYSTICK INTERRUPT HANDLER//////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SysTick_Handler(void)
{ 
	second+=1;				// Count intervals of 100ms
	if(second==10)
	{
		second=0;
		second_OK=1;		// the one second has passed
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////UART INTERRUPT HANDLER////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UART0_IRQHandler()
{
	if(UART0->S1 & UART0_S1_RDRF_MASK)
	{
		tempo=UART0->D;	// Reading the value from the receiver's buffer and clearing the RDRF flag
		if(!rx_FULL)
		{
			if(tempo!=CRET)
			{
				if(!too_long)	// If the string is too long, ignore the rest of the characters
				{
					rx_buf[rx_buf_pos] = tempo;	// Complete the command
					rx_buf_pos++;
					if(rx_buf_pos==16)
						too_long=1;		// Too long a string
				}
			}
			else
			{
				if(!too_long)	// If the string is too long, drop the tables
					rx_buf[rx_buf_pos] = 0;
				rx_FULL=1;
			}
		}
	NVIC_EnableIRQ(UART0_IRQn);
	}
}


int main (void)
{
	char check; // Check if admin password is correct
	char check_user; // Check if user password is correct
	uint8_t	kal_error; // For converter A/D calibration
	
	// Init modules
	UART0_Init();		// Initialization ports
	Klaw_Init();
	RTC_Init();
	LCD1602_Init();
	PWM_Init();				// Initialization of the TPM0 counter (PWM "Low-true pulses")

	LCD1602_Backlight(TRUE);
	
	time_write(18, 8, 0, 18, January, 2022, Tuesday); // Write to data structure of time
	time_read(RTC_Read()); // read actuall time 	
	
	// Calibration A/D
	kal_error=ADC_Init();				// Initialization and calibration A/C converter
	if(kal_error)
	{	
		LCD1602_SetCursor(0,0);
		LCD1602_Print("CALIBRATION");
		LCD1602_SetCursor(0,0);
		LCD1602_Print("ERROR");
		while(1);									// calibration is not correct
	}
	
	// ADC trigger
	ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(3);		// First trigger ADC0 on channel 8 and enable the interrupt
	
	// Start message on terminal
	sprintf(display,"BLUETOOTH OFF%c%c",CRET,LF); 
	for(int i=0;display[i]!=0;i++)
	{
		while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
		UART0->D = display[i];		// send to user
	}
	sprintf(display,"TURN ON MODULE%c%c%c",CRET,LF,LF); 
	for(int i=0;display[i]!=0;i++)
	{
		while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
		UART0->D = display[i];		// send to user
	}
	
	
	// Start message display on LCD
	LCD1602_Print("ALARM UNARMED!");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S1 TO CONTINUE");
	while(PTA->PDIR&C1_MASK);		// Wait to press SW1
	contact_vibration();		// Reduce contact vibration
	LCD1602_ClearAll();
	
	SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)
	
	UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK );		//Lock the transmitter and receiver to disable BLUETOOTH communication until enabled

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
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////IF SW1 IS PRESSED ENTER CORRECT PASS AND GO TO THE ADMIN MENU////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			case SW1:
				SysTick_Config(1); // Stop the countdown
				time_count = 0; // reset systick counter when user is not afk and using device
				second_OK = 0;
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
						else if (check == 2) // Typing cancelled
						{
							LCD1602_Print("CANCELLED");
							DELAY(500)
							break;
						}	
					}	
				}
				else if (check == 1) // Correct password then go to the admin menu
					admin_setup();
				else if (check == 2) // Typing cancelled
				{
					LCD1602_Print("CANCELLED");
					DELAY(500)
				}
				SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)
				break;
				
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////IF SW2 IS PRESSED ENTER CORRECT PASS AND GO TO THE USER MENU/////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			case SW2:
				SysTick_Config(1); // Stop the countdown
				time_count = 0; // reset systick counter when user is not afk and using device
				second_OK = 0;				
				check_user = enter_user_passwd();
				if (check_user==0)	//wrong password
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
						else if (check_user == 1) // Correct password then go to the admin menu
						{
							user_setup();
							break;
						}	
						else if (check == 2) // Typing cancelled
						{
							LCD1602_Print("CANCELLED");
							DELAY(500)
							break;
						}							
					}	
				}
				else if (check_user == 1) // Correct password then go to the admin menu
					user_setup();
				else if (check == 2) // Typing cancelled
				{
					LCD1602_Print("CANCELLED");
					DELAY(500)
				}
				SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)				
				break;
				
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////IF SW3 IS PRESSED ENTER CORRECT PASS AND GO TO THE BLUETOOTH MENU//////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			case SW3:
				SysTick_Config(1); // Stop the countdown
				time_count = 0; // reset systick counter when user is not afk and using device
				second_OK = 0;	
				check = enter_passwd();
				if (check==0)	//wrong password
				{
					LCD1602_ClearAll();
					LCD1602_Print("WRONG PASSWORD");
					DELAY(1000)
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
							UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);		//Turn on the transmitter and receiver
							LCD1602_ClearAll();
							LCD1602_Print("BLUETOOTH ON");
							DELAY(1000)
							sprintf(display,"BLUETOOTH ON%c%c",CRET,LF); 
							for(int i=0;display[i]!=0;i++)
							{
									while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
									UART0->D = display[i];		// send to user
							}
							sprintf(display,"Type ALARMON:%c%c",CRET,LF); 
							for(int i=0;display[i]!=0;i++)
							{
									while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
									UART0->D = display[i];		// send to user
							}
							bluetooth_on = 1;
							break;
						}
						else if(keyread==SW2) // TURN OFF BLUETOOTH MODULE
						{
							LCD1602_ClearAll();
							LCD1602_Print("BLUETOOTH OFF");
							DELAY(1000)
							sprintf(display,"BLUETOOTH OFF%c%c",CRET,LF); 
							for(int i=0;display[i]!=0;i++){
									while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
									UART0->D = display[i];		// send to user
							}
							sprintf(display,"TURN ON MODULE%c%c%c",CRET,LF,LF); 
							for(int i=0;display[i]!=0;i++){
									while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
									UART0->D = display[i];		// send to user
							}
							bluetooth_on = 0;
							UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK );		// Locking the transmitter and receiver
							break;
						}
						else if(keyread==SW16) // Exit from this menu
						{
							LCD1602_ClearAll();
							break;
						}
					}//end while(1)	
				}
				SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)
				break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/////////////////////////////IF SW4 IS PRESSED ENTER CORRECT PASS AND GO TO THE CLOCK MENU/////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
			case SW4:
				SysTick_Config(1); // Stop the countdown
				time_count = 0; // reset systick counter when user is not afk and using device
				second_OK = 0;	
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
						time_read(RTC_Read()); // read actuall time 
						break;
					}
					else if(keyread==SW2) // display time 
					{
						LCD1602_ClearAll();
						while(1)
						{
							time_read(RTC_Read()); // read actuall time 
								
							sprintf(display,"%02d.%02d.%04dr",data.tm_mday,data.tm_mon+1,data.tm_year+1900);
							LCD1602_SetCursor(0,0);
							LCD1602_Print(display);
							sprintf(display,"%02d:%02d:%02d",data.tm_hour,data.tm_min,data.tm_sec);
							LCD1602_SetCursor(0,1);
							LCD1602_Print(display);
								
							keyread = read_keypad();
							if (keyread==SW16) 		// Cancell
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
				SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)
				break;
		}//end switch
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////ARMING DEVICE FROM THE TERMINAL////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if(rx_FULL)		// Data is ready?
		{
			SysTick_Config(1); // Stop the countdown
			time_count = 0; // reset systick counter when user is not afk and using device
			second_OK = 0;	
			if(too_long)
			{
				for(int i=0;Too_Long[i]!=0;i++)	// too long string
				{
					while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
					UART0->D = Too_Long[i];
				}
				while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
				UART0->D = LF;		// enter
				while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
				UART0->D = CRET;		// carriage return
				too_long=0;
			}
			else
			{
				if(strcmp (rx_buf,ALARM_ON)==0)	// Arm alarm 
					armed();
				else
				{
					for(int i=0;Error[i]!=0;i++)	// wrong command
					{
						while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
						UART0->D = Error[i];
					}
					while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
					UART0->D = LF;		// enter
					while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
					UART0->D = CRET;		// carriage return
				}
			}
			rx_buf_pos=0;
			rx_FULL=0;	// consumed data
			SysTick_Config(SystemCoreClock/10 );	// SysTick counter start (interrupt every 100ms)
		}	
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////AFTER A ONE MINUTE OF INACTIVITY, THE DISPLAY SHOWS THE TIME//////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		if(second_OK)
		{
			time_count+=1;
			if(time_count==60)		// 60 seconds
			{
				LCD1602_ClearAll();
				while(1)
				{
					time_read(RTC_Read()); // read actuall time 
								
					sprintf(display,"%02d.%02d.%04dr",data.tm_mday,data.tm_mon+1,data.tm_year+1900);
					LCD1602_SetCursor(0,0);
					LCD1602_Print(display);
					sprintf(display,"%02d:%02d:%02d",data.tm_hour,data.tm_min,data.tm_sec);
					LCD1602_SetCursor(0,1);
					LCD1602_Print(display);
								
					keyread = read_keypad();
					if (keyread>0) 		// Cancell
					{
						
						LCD1602_ClearAll();
						break;
					}
				}
				time_count=0;
			}	
			second_OK=0;
		}
		
	}
}
