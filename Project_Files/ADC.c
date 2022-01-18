#include "ADC.h"

uint8_t ADC_Init(void)
{
	uint16_t kalib_temp;
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;          // clock for ADC0
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;					// clock for PORT A
	PORTB->PCR[8] &= ~(PORT_PCR_MUX(2));		// PTA8 - analog input, channel 3
	ADC0->CFG1 = ADC_CFG1_ADICLK(ADICLK_BUS_2) | ADC_CFG1_ADIV(ADIV_4) | ADC_CFG1_ADLSMP_MASK;	// input clock BUS/2=10.49MHz, clock ADCK is 2.62MHz (2621440Hz), long sampling time
	ADC0->CFG2 = ADC_CFG2_ADHSC_MASK;										// Turn on the high-speed clock assist
	ADC0->SC3  = ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(3);		// Turn on averaging over 32 samples
	ADC0->SC3 |= ADC_SC3_CAL_MASK;											// Start calibration
	while(ADC0->SC3 & ADC_SC3_CAL_MASK);								// Wait for the end of calibration
	
	if(ADC0->SC3 & ADC_SC3_CALF_MASK)
	{
	  ADC0->SC3 |= ADC_SC3_CALF_MASK;
	  return(1);																				// Return if calibration error
	}
	
	kalib_temp = 0x00;
	kalib_temp += ADC0->CLP0;
	kalib_temp += ADC0->CLP1;
	kalib_temp += ADC0->CLP2;
	kalib_temp += ADC0->CLP3;
	kalib_temp += ADC0->CLP4;
	kalib_temp += ADC0->CLPS;
	kalib_temp += ADC0->CLPD;
	kalib_temp /= 2;
	kalib_temp |= 0x8000;                       // Set the most significant bit to 1
	ADC0->PG = ADC_PG_PG(kalib_temp);           // Save to "plus-side gain calibration register"
	//ADC0->OFS = 0;														// Zero shift calibration (from measuring your reference point - mass)
	ADC0->SC1[0] = ADC_SC1_ADCH(31);						// Lock the ADC0 converter
	ADC0->CFG2 |= ADC_CFG2_ADHSC_MASK;					// Enable fast conversion mode
	ADC0->CFG1 = ADC_CFG1_ADICLK(ADICLK_BUS_2) | ADC_CFG1_ADIV(ADIV_1) | ADC_CFG1_ADLSMP_MASK | ADC_CFG1_MODE(MODE_12);	// clock ADCK równy 10.49MHz, resolution 12 bits, long sampling time
	ADC0->SC3 |= ADC_SC3_ADCO_MASK;							// Continuos conversion
	NVIC_ClearPendingIRQ(ADC0_IRQn);
	NVIC_EnableIRQ(ADC0_IRQn);
	return(0);																
}

