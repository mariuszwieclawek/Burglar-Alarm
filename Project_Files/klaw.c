#include "klaw.h"
#include "frdm_bsp.h"

/* Ports init */
void Klaw_Init(void)
{
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;		// Enable port A
	/* Columns as inputs */
	PORTA->PCR[COL1] |= PORT_PCR_MUX(1);
	PORTA->PCR[COL2] |= PORT_PCR_MUX(1);
	PORTA->PCR[COL3] |= PORT_PCR_MUX(1);
	PORTA->PCR[COL4] |= PORT_PCR_MUX(1);
	PORTA->PCR[COL1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; //The PE bit enables the use of a secondary resistor, and the PS bit selects how this resistor is to be connected.
	PORTA->PCR[COL2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[COL3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[COL4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	
	/* Rows as outputs */
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;		// Enable port B
	PORTB->PCR[ROW1] |= PORT_PCR_MUX(1);
	PORTB->PCR[ROW2] |= PORT_PCR_MUX(1);
	PORTB->PCR[ROW3] |= PORT_PCR_MUX(1);
	PORTB->PCR[ROW4] |= PORT_PCR_MUX(1);
	PTB->PDDR |= R1_MASK|R2_MASK|R3_MASK|R4_MASK;	// Set to 1 bits 6, 7, 11, 13 - role as output
	PTB->PDOR |= R1_MASK|R2_MASK|R3_MASK;	// Initially, the three outputs are set to high voltage.
	PTB->PDOR &= ~R4_MASK;   // First row set to low voltage
}


/* The function returns the values of the pressed key in the order 1-16. For S1 return 1; For S8 return 8; */
unsigned int read_keypad(void) 
{
	unsigned int row,col,key;
	
	for(row = 0x1fff, key = 1; row != 0x3fdf; row = (row >> 1)|(1<<13))
	{
		if(row == 0x2fff) // Omit PTB12
			continue;
		else if(row == 0x3bff) // Omit PTB10
			continue;
		else if(row == 0x3dff) // Omit PTB9
			continue;
		else if(row == 0x3eff) // Omit PTB8
			continue;
		PTB->PDOR = row ;
		for(col=0x200; col!=0x2000; col<<=1, key++)
		{
			if(!(PTA->PDIR & col)) 
				return key;
		}	
	}
	return 0;
}


/* This function reducing contact vibration of switches*/
void contact_vibration(void)
{
	while(read_keypad()); // Waiting for the button state to change
	DELAY(80)  // Wait until the contacts vibration is stopped
}

