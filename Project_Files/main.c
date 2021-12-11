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

unsigned int keyread = 0;
char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
char tx_buf[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

char password[] = {1,2,3,4};

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};


void alarm(void);
void admin_setup(void);


/* The function which shows the procedure of entering the password on the LCD1602, unset alarm */
void typing_passwd(char cursor_pos)
{	
	PORTB->PCR[10] |= PORT_PCR_MUX(1);					//  Selection of the function to be performed by the given pin of port B
	PTB->PDDR |= (1<<10);	// Set to 1 bit 10 - role as outputs
	PTB->PCOR|= (1<<10);	// Diode ON
	LCD1602_SetCursor(cursor_pos,1);
	LCD1602_Print("X");
}


/* This function is using to entering the password*/
/*				return 1 if the password is correct			*/
/*				return 0 if the password is incorrect			*/
char enter_passwd(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("ENTER PASSWORD:");
	
	char i,cursor_pos;
	char err=0;
	
	for(i=0,cursor_pos=0; i<4; i++,cursor_pos++)
	{
		while(!(keyread = read_keypad())); // Waiting for a keyboard button to be pressed
		contact_vibration(); // deleting of contact vibration
					
		if(keyread == SW16) // if s16 delete of password typing
		{ 
			LCD1602_ClearAll();
			break;
		}
				
		if(keyread != password[i]) err++; // check if the password is correct, unless err+
				
		if(i==3) // last digit
		{ 
			switch(err)
			{
				case 0: // correct pass
					return 1;
				default: // incorrect pass
					return 0;
			}
		}
		else
			typing_passwd(cursor_pos); // unarmed alarm, password input progress
	}
}



/* The function that is responsible for arming our alarm */
void armed(void)
{	
	PORTB->PCR[9] |= PORT_PCR_MUX(1);					// Selection of the function to be performed by the given pin of port B
	PTB->PDDR |= (1<<9);	// Set to 1 bit 9 - role as outputs
	LCD1602_ClearAll();
	LCD1602_Print("ALARM IS ARMED");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S16:OFF");
	char exit; // using to exit infinite loop
	while(1)
	{
		PTB->PCOR|= (1<<9);	// Diode ON
		keyread = read_keypad();	// Reading from 4x4 tact switches and write to keyread
		
		if (keyread==SW16) // Disarming if you press SW16
		{
			LCD1602_ClearAll();
			PTB->PSOR|= (1<<9);	// Diode OFF
			contact_vibration();
			char check = enter_passwd(); // Enter the password and check if its correct or incorrect
			
			switch(check)
			{
				case 0: // wrong password
					for(char chances = 1; chances != 4; chances++)	// you have 3 chances to put correct password, then alarm will start
					{
						LCD1602_ClearAll();
						LCD1602_Print("WRONG PASSWD");
						LCD1602_SetCursor(0,1);
						LCD1602_Print("TRY AGAIN S16:EX");
						DELAY(1000)
						check = enter_passwd();
						if(chances==2 && check ==0)	//wrong password after 3 chances start alarming
						{
							alarm();
							exit = 1;
							break;
						}
						else if (check == 1) //correct password
						{
							exit = 1;
							break;
						}		
					}	
				case 1: // correct password
					exit = 1;
					break;
			} // end switch(check)
		} // end if (keyread==SW16)
		if(exit == 1) 
			break;
	} //end while
}


/* This function is responsible for changing password*/
char* change_passwd(char pass[])
{
	contact_vibration(); // deleting of contact vibration
	LCD1602_ClearAll();
	LCD1602_Print("CHANGE PASSWORD:");
	LCD1602_SetCursor(10,1);
	LCD1602_Print("S16:EX");
	LCD1602_SetCursor(0,1);
	char k,cursor_pos;			// cursos_pos is using to displaying typing progress on LCD1602
	char exit=0; 		//exit is using to exit of typing new password, return the old password
	char new_password[4]; // temporary password , used to set a password when progress is not cancelled
	
	for(k=0,cursor_pos=0; k<4; k++,cursor_pos++)
	{
		while(!(keyread = read_keypad())); // Waiting for a keyboard button to be pressed
		if(keyread == SW16){ // Exit when press SW16
			exit = 1;
			break;
		}
		contact_vibration(); // deleting of contact vibration
		new_password[k] = keyread; // new password from tact switches
		typing_passwd(cursor_pos); // typing progress
	}
	
	if(exit == 1) // cancelled typing
	{
		LCD1602_ClearAll();
		LCD1602_Print("YOU CANCELLED OF");
		LCD1602_SetCursor(0,1);
		LCD1602_Print("TYPING PASSWD");
		DELAY(2000)
		return pass;
	}
		
	else	// You set a new password
	{
		LCD1602_ClearAll();
		LCD1602_Print("YOU SET A NEW");
		LCD1602_SetCursor(0,1);
		LCD1602_Print("PASSWORD");
		DELAY(2000)
		for(k = 0; k<4; k++)
			pass[k] = new_password[k]; // copying password
		return pass;
	}
}

void admin_setup(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("S1:START ALARM");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S2:CHANGE PASSWD");
	while(1)
	{
		keyread = read_keypad();
		if (keyread==SW1) //start alarm
		{
			armed();
			break;
		}
		else if(keyread==SW2) // change password
		{
			change_passwd(password);
			break;
		}
		else if(keyread==SW16) // Exit from this menu
		{
			LCD1602_ClearAll();
			break;
		}
	}
}


/* Alarm signaling function on diode, buzzer and 7seg displays // LED and buzzer on one port */
void alarm(void)
{	
	PORTB->PCR[8] |= PORT_PCR_MUX(1);					// Selection of the function to be performed by the given pin of port B
	PORTB->PCR[10] |= PORT_PCR_MUX(1);				
	PTB->PDDR |= (1<<8);	// Set to 1 bit 9 - role as outputs
	PTB->PDDR |= (1<<10);	
	LCD1602_ClearAll();
	LCD1602_Print("*****ALARM!*****");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("**HOLD:S16:OFF**");
	while(1)
	{
		PTB->PCOR|= (1<<8);	// Diode ON
		DELAY(200)
		PTB->PSOR|= (1<<8);	// Diode OFF
		PTB->PCOR|= (1<<10);	// Diode ON
		DELAY(200)
		PTB->PSOR|= (1<<10);	// Diode OFF
		keyread = read_keypad();
		if (keyread==SW16) 		// Cancelling the alarm 
		{
			contact_vibration();
			LCD1602_ClearAll();
			PTB->PSOR|= (1<<8);	// Diode OFF
			PTB->PSOR|= (1<<10);	// Diode OFF
			char check = enter_passwd();
				if (check==0)	//wrong password still alarming
				{	
					LCD1602_ClearAll();
					LCD1602_Print("*****ALARM!*****");
					LCD1602_SetCursor(0,1);
					LCD1602_Print("**HOLD:S16:OFF**");
					continue;
				}
				else if (check == 1) //if good password stop alarming
					break; 
		}
	}
}



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
