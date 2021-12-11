/*
Keyboard 4x4 tact switches init and function to read which switch is pushed
*/

#ifndef KLAW_H
#define KLAW_H


#include "MKL05Z4.h"

#define C1 9		// first column 4x4 tact switches
#define C2 10
#define C3 11
#define C4 12

#define R1 6		// first row 4x4 tact switches
#define R2 7
#define R3 11
#define R4 13


#define C1_MASK	(1<<9)		// Mask for C4 column
#define C2_MASK	(1<<10)		
#define C3_MASK	(1<<11)		
#define C4_MASK	(1<<12)		

#define R1_MASK	(1<<6)		// Mask for R1 row
#define R2_MASK	(1<<7)		
#define R3_MASK	(1<<11)		
#define R4_MASK	(1<<13)		


void Klaw_Init(void);		// Ports init
unsigned int read_keypad(void);		//The function returns the values of the pressed key in the order 1-16. For S1 return 1; For S8 return 8;
void contact_vibration(void); // This function reducing contact vibration of switches

#endif