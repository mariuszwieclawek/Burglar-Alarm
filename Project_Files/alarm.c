/* Functions for handling the alarm */

#include "frdm_bsp.h"
#include "alarm.h"
#include "klaw.h"
#include "i2c.h"
#include "lcd1602.h"

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};
extern char keyread;
extern char password[];


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

/* The function allows you to go to the administrator menu */
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


/* Alarm signaling function */
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
