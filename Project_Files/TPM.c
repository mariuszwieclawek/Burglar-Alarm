#include "TPM.h"
void PWM_Init()
{
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;			// Dołączenie sygnału zegara do portu B
	// Wybór odpowiedniej roli pinu portu B
	PORTB->PCR[9] |= PORT_PCR_MUX(2);		// LED G	TPM0_CH2

	SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;				// Dołączenie sygnału zegara do TPM0
	SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);			  // Wybierz źródło taktowania TPMx MCGFLLCLK=41943040Hz
	
	TPM0->SC &= ~TPM_SC_CPWMS_MASK;					//	TPM0 w trybie zliczanie "w przód"
	TPM0->SC |= TPM_SC_PS(5);								//	Dzielnik zegara wejściowegoo równy 32; zegar=1310720Hz
	TPM0->MOD = 0xFFFF;													//	Rejestr MODULO=65535 - fwy=20Hz
	TPM0->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, kanał 1 (LED niebieski) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	TPM0->CONTROLS[1].CnV = 0x0000;					// Wsółczynnik wypełnienia początkowo 0
	TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, kanał 2 (LED zielony) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	TPM0->CONTROLS[2].CnV = 0x0000;					// Wsółczynnik wypełnienia początkowo 0
	TPM0->CONTROLS[3].CnSC = TPM_CnSC_MSB_MASK|TPM_CnSC_ELSA_MASK;	//	TPM0, kanał 3 (LED czerwony) tryb "Edge-aligned PWM Low-true pulses (set Output on match, clear Output on reload)"
	TPM0->CONTROLS[3].CnV = 0x0000;					// Wsółczynnik wypełnienia początkowo 0
	TPM0->SC |= TPM_SC_CMOD(1);							// Włącz licznik TPM0
}
