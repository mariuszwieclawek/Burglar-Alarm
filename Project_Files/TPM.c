#include "TPM.h"
void PWM_Init()
{
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;			// clock for PORT B
	// function of pin
	PORTB->PCR[9] |= PORT_PCR_MUX(2);		// LED G	TPM0_CH2

	SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;				// clock for TPM0
	SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);			  // clock source TPMx MCGFLLCLK=41943040Hz
	
	TPM0->SC &= ~TPM_SC_CPWMS_MASK;					//	TPM0 in "forward" counting mode
	TPM0->SC |= TPM_SC_PS(5);								//	prescaler = 32; clock=1310720Hz
	TPM0->MOD = 0xFFFF;													//	Register MODULO=65535 - fwy=20Hz
	//TPM0->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, channel 1 (LED blue) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	//TPM0->CONTROLS[1].CnV = 0x0000;					// Filling factor initially 0
	TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, channel 2 (LED green) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	TPM0->CONTROLS[2].CnV = 0x0000;					// Filling factor initially 0
	//TPM0->CONTROLS[3].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, channel 3 (LED red) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	//TPM0->CONTROLS[3].CnV = 0x0000;					// Filling factor initially 0
	TPM0->SC |= TPM_SC_CMOD(1);							// Turn on counter TPM0
}
