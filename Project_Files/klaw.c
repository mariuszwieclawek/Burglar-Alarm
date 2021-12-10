#include "klaw.h"


void Klaw_Init(void)
{
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;		// Wlaczenie portu A
	/* Kolumny jako wejscia */
	PORTA->PCR[C1] |= PORT_PCR_MUX(1);
	PORTA->PCR[C2] |= PORT_PCR_MUX(1);
	PORTA->PCR[C3] |= PORT_PCR_MUX(1);
	PORTA->PCR[C4] |= PORT_PCR_MUX(1);
	PORTA->PCR[C1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; //Bit PE wlacza mozliwosc uzycia rezystora dodatkowego, a bit PS wybiera, jak ten rezystor ma byc podlaczony.
	PORTA->PCR[C2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[C3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[C4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	
	/* Wiersze jako wyjscia */
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;		// Wlaczenie portu B
	PORTB->PCR[R1] |= PORT_PCR_MUX(1);
	PORTB->PCR[R2] |= PORT_PCR_MUX(1);
	PORTB->PCR[R3] |= PORT_PCR_MUX(1);
	PORTB->PCR[R4] |= PORT_PCR_MUX(1);
	PTB->PDDR |= R1_MASK|R2_MASK|R3_MASK|R4_MASK;	// Ustaw na 1 bity 6, 7 , 11, 13 – rola jako wyjscia
	PTB->PDOR |= R1_MASK|R2_MASK|R3_MASK;	// Poczatkowo trzy wyjscia ustawiana sa na stan wysoki napiecia.
	PTB->PDOR &= ~R4_MASK;   // Pierwszy wiersz na stan niski 
}

unsigned int read_keypad(void) 
{
	unsigned int row,col,key;
	
	for(row = 0x1fff, key = 1; row != 0x3fdf; row = (row >> 1)|(1<<13))
	{
		if(row == 0x2fff) //Pomijamy PTB12
			continue;
		else if(row == 0x3bff) //Pomijamy PTB10
			continue;
		else if(row == 0x3dff) //Pomijamy PTB9
			continue;
		else if(row == 0x3eff) //Pomijamy PTB8
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
