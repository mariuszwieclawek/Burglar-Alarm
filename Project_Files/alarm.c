/* Functions for handling the alarm */
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "frdm_bsp.h"
#include "alarm.h"
#include "klaw.h"
#include "i2c.h"
#include "lcd1602.h"
#include "ADC.h"
#include "TPM.h"
#include "RTCclock.h"

#define ALARM_SENS 1 // Light intensity voltage level, when is above alarm start
#define CRET	0xd		// Carriage return
#define LF	0xa		// Enter

#define	ZYXDR_Mask	1<<3	// The mask of the ZYXDR bit in the STATUS register (accelerometer)

enum menu{START,SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13,SW14,SW15,SW16};
extern char keyread;
extern char password[];
extern char user_password[];
extern char display[];

// A/D CONVERTER
float adc_volt_coeff = ((float)(((float)2.91) / 4095) );			// Correction factor of the result in relation to the reference voltage of the converter
uint8_t result_ok=0;
uint16_t temp;
float	result;

//UART
extern char Too_Long[];
extern char Error[];
extern char ALARM_ON[];
extern char ALARM_OFF[];
extern char rx_buf[16];
extern uint8_t rx_buf_pos;
extern char tempo,buf;
extern uint8_t rx_FULL;
extern uint8_t too_long;
extern char bluetooth_on;

//ACCELEROMETR
static uint8_t arrayXYZ[6];
static uint8_t sens;
static uint8_t status;
double X, Y, Z;


void ADC0_IRQHandler()
{	
	temp = ADC0->R[0];	// Reading the data and deleting the COCO flag
	if(!result_ok)				// Check that the result consumed by the main loop
	{
		result = temp;			// Send new data to the main loop
		result_ok=1;
	}
}


/* The function which shows the procedure of entering the password on the LCD1602, unset alarm */
void typing_passwd(char cursor_pos)
{	
	PORTB->PCR[10] |= PORT_PCR_MUX(1);					//  Selection of the function to be performed by the given pin of port B
	PTB->PDDR |= (1<<10);	// Set to 1 bit 10 - role as outputs
	PTB->PCOR|= (1<<10);	// Diode ON
	LCD1602_SetCursor(cursor_pos,1);
	LCD1602_Print("X");
}


/* This function is using to entering the admin password*/
/*				return 0 if the password is incorrect			*/
/*				return 1 if the password is correct			*/
/*				return 2 if the typing is cancelled			*/
char enter_passwd(void)
{
	contact_vibration();
	LCD1602_ClearAll();
	LCD1602_Print("ENTER PASSWORD:");
	
	char i,cursor_pos;
	char err=0;
	
	for(i=0,cursor_pos=0; i<4; i++,cursor_pos++)
	{
		while(!(keyread = read_keypad())); // Waiting for a keyboard button to be pressed
		contact_vibration(); // deleting of contact vibration
					
		if(keyread == SW16) // if s16 cancell of password typing
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
	return 2; // typing cancelled
}

/* This function is using to entering the user password*/
/*				return 0 if the password is incorrect			*/
/*				return 1 if the password is correct			*/
/*				return 2 if the typing is cancelled			*/
char enter_user_passwd(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("ENTER PASSWORD:");
	
	char i,cursor_pos;
	char err=0;
	
	for(i=0,cursor_pos=0; i<4; i++,cursor_pos++)
	{
		while(!(keyread = read_keypad())); // Waiting for a keyboard button to be pressed
		contact_vibration(); // deleting of contact vibration
					
		if(keyread == SW16) // if s16 cancell of password typing
		{ 
			LCD1602_ClearAll();
			break;
		}
				
		if(keyread != user_password[i]) err++; // check if the password is correct, unless err+
				
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
	return 2; // typing cancelled
}



/* The function that is responsible for arming our alarm */
void armed(void)
{	
	LCD1602_ClearAll();
	LCD1602_Print("ALARM IS ARMED");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("S16:OFF");
	
	if(bluetooth_on) // When bluetooth is on
	{
		sprintf(display,"ALARM IS ARMED%c%c",CRET,LF); 
		for(int i=0;display[i]!=0;i++)
		{
			while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
			UART0->D = display[i];		// send to user
		}
		sprintf(display,"Type ALARMOFF%c%c",CRET,LF); 
		for(int i=0;display[i]!=0;i++)
		{
			while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter empty?
			UART0->D = display[i];		// send to user
		}
	}
	
	char exit=0; // using to exit infinite loop
	rx_FULL=0;
	rx_buf_pos=0;
	
	// ACCELEROMETR SETUP
	sens=0;	// Sensivity select: 0 - 2g; 1 - 4g; 2 - 8g
	I2C_WriteReg(0x1d, 0x2a, 0x0);	// ACTIVE=0 - standby
	I2C_WriteReg(0x1d, 0xe, sens);	 		// Set the sensitivity according to the sense variable
	I2C_WriteReg(0x1d, 0x2a, 0x1);	 		// ACTIVE=1 - active state
	
	while(1)
	{
		time_read(RTC_Read()); // read actuall time 
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////TURN OFF ALARM STANDBY FROM THE TERMINAL OF THE BLUETOOTH DEVICE//////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if(rx_FULL && bluetooth_on)		// If is data ready and bluetooth turned on?
		{
			if(too_long) // when the text you put is too long
			{
				for(int i=0;Too_Long[i]!=0;i++)
				{
					while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
					UART0->D = Too_Long[i]; // inform the user about this error
				}
				while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
				UART0->D = LF;		// enter
				while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
				UART0->D = CRET;		// carriage return
				too_long=0;
			}
			else
			{
				if(strcmp (rx_buf,ALARM_OFF)==0) // Turn off the alarm standby
				{
					LCD1602_ClearAll();
					LCD1602_Print("ALARM TURNS OFF");
					sprintf(display,"ALARM TURNS OFF%c%c",CRET,LF); 
					for(int i=0;display[i]!=0;i++)
					{
						while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Is the transmitter ready?
						UART0->D = display[i];		// inform the user if the alarm has been disabled
					}
					exit = 1;
					DELAY(1000)
				}
				else
				{
					for(int i=0;Error[i]!=0;i++)	// the user entered the wrong command
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
		}		
		
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////TURN ON ALARM SIGNALLING WHEN IT IS TOO BRIGHT///////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if(result_ok) // If the value from converter is ready
		{
			result = result*adc_volt_coeff;		// Value as a voltage
			if(result>ALARM_SENS)		// When the value from light sensor is above the maximum 
			{
				alarm(); // ALARM
				break;
			}
			result_ok=0;	
		}
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////TURN ON ALARM SIGNALLING WHEN YOU MOVE DEVICE////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		I2C_ReadReg(0x1d, 0x0, &status);
		status&=ZYXDR_Mask;
		if(status)	// Is the data readable?
		{
			I2C_ReadRegBlock(0x1d, 0x1, 6, arrayXYZ);
			X=((double)((int16_t)((arrayXYZ[0]<<8)|arrayXYZ[1])>>2)/(4096>>sens));
			Y=((double)((int16_t)((arrayXYZ[2]<<8)|arrayXYZ[3])>>2)/(4096>>sens));
			Z=((double)((int16_t)((arrayXYZ[4]<<8)|arrayXYZ[5])>>2)/(4096>>sens));
			
			if(fabs(X)>1.5 || fabs(Y)>1.5 || fabs(Z)>1.5)
			{
				alarm();
				break;
			}
		}
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////TURN OFF ALARM////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		keyread = read_keypad();	// Reading from 4x4 tact switches and write to keyread
		if (keyread==SW16) // Disarming if you press SW16
		{
			LCD1602_ClearAll();
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
						LCD1602_Print("TRY AGAIN");
						DELAY(1000)
						check = enter_passwd();
						
						if (check == 2) //typing cancelled
						{
							LCD1602_ClearAll();
							LCD1602_Print("ALARM IS ARMED");
							LCD1602_SetCursor(0,1);
							LCD1602_Print("S16:OFF");
							exit = 0;
							break;
						}			
						else if(chances==2 && check ==0)	//wrong password after 3 chances start alarming
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
					break;
				case 1: // correct password
					exit = 1;
					break;
				case 2: // typing cancelled
					LCD1602_ClearAll();
					LCD1602_Print("ALARM IS ARMED");
					LCD1602_SetCursor(0,1);
					LCD1602_Print("S16:OFF");
					exit = 0;
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

/* The function allows you to go to the user menu */
void user_setup(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("S1:START ALARM");
	while(1)
	{
		keyread = read_keypad();
		if (keyread==SW1) //start alarm
		{
			armed();
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
	uint16_t	mod_curr;
	int up_down = 1;
	int divider =64;
	int freq = 200000;
	
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
		DELAY(20)
		PTB->PSOR|= (1<<8);	// Diode OFF
		PTB->PCOR|= (1<<10);	// Diode ON
		DELAY(20)
		PTB->PSOR|= (1<<10);	// Diode OFF
		
		if(up_down)
		{
			mod_curr=freq/divider;
			TPM0->MOD = mod_curr;
			divider+=1;
			if(divider==128)
				up_down = 0;
			TPM0->CONTROLS[2].CnV = mod_curr*0.5;	// Nowa wartosc kreujaca wspólczynnik wypelnienia PWM
		}
		else if(up_down == 0)
		{
			mod_curr=freq/divider;
			TPM0->MOD = mod_curr;
			divider-=1;
			if(divider==64)
				up_down = 1;
			TPM0->CONTROLS[2].CnV = mod_curr*0.5;	// Nowa wartosc kreujaca wspólczynnik wypelnienia PWM
		}
		
		keyread = read_keypad();
		if (keyread==SW16) 		// Cancelling the alarm 
		{
			contact_vibration();
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
				{
					LCD1602_ClearAll();
					PTB->PSOR|= (1<<8);	// Diode OFF
					PTB->PSOR|= (1<<10);	// Diode OFF
					TPM0->CONTROLS[2].CnV = 0;
					break; 
				}			
		}
	}
}
