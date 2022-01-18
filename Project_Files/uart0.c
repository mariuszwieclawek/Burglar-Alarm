#include "uart0.h"

void UART0_Init(void)
{
	SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;							// clock for UART0
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;							// clock for PORT B
	SIM->SOPT2 |= SIM_SOPT2_UART0SRC(MCGFLLCLK);		// Clock MCGFLLCLK=41943040Hz (CLOCK_SETUP=0)
	PORTB->PCR[1] = PORT_PCR_MUX(2);								// PTB1=TX_D
	PORTB->PCR[2] = PORT_PCR_MUX(2);								// PTB2=RX_D
	
	UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK );		// Disable the transmitter and receiver
	UART0->BDH = 1;			// for CLOCK_SETUP=0 BR=28800	BDH=0 BR=9600	BDH=1
	UART0->BDL =17;			// for CLOCK_SETUP=0 BR=28800	BDL=91 BR=9600	BDL=17	:	CLOCK_SETUP=1	BR=230400		BDL=13
	UART0->C4 &= ~UART0_C4_OSR_MASK;
	UART0->C4 |= UART0_C4_OSR(15);	// for CLOCK_SETUP=0 BR=28800	OSR=15	:	CLOCK_SETUP=1	BR=230400		OSR=15
	UART0->C5 |= UART0_C5_BOTHEDGE_MASK;	// Receiver sampling on both clock edges
	UART0->C2 |= UART0_C2_RIE_MASK;		// Enable interrupts from receiver
	UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);		// Turn on the transmitter and receiver
	NVIC_EnableIRQ(UART0_IRQn);
	NVIC_ClearPendingIRQ(UART0_IRQn);
}
