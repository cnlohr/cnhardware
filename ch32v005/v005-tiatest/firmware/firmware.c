/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

int main()
{
	SystemInit();

	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;

	// NSEL1 = 001 (PD0) = OPN1
	// MODE1 = 00 (PD4)
	// NSEL1 must be disconnected( how?)
	// PSEL  = 01 (PD7)

	funPinMode( PD4, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // OPO
	funPinMode( PD7, GPIO_CFGLR_IN_FLOAT ); // OPP1
	funPinMode( PD0, GPIO_CFGLR_IN_FLOAT ); // OPN1

	// right now, 22k 1pF

	funPinMode( PD7,GPIO_CFGLR_OUT_50Mhz_AF_PP ); // OPP1 OVerride rail
	funDigitalWrite( PD7, 0 );

	// Mode 17.2.1.1 OPA Single-ended Input
	OPA->OPA_KEY = 0x45670123; // KEY1
	OPA->OPA_KEY = 0xCDEF89AB; // KEY2
	//OPA->CFGR1 = 0;
	OPA->CTLR1 = 
		OPA_CTLR1_OPA_HS1 |
		OPA_CTLR1_OPA_LOCK |
		OPA_CTLR1_NSEL1_CHN1 | // NSEL1 = PD0
		OPA_CTLR1_PSEL1_CHP1 | // PSEL1 = PD7
		0 | // MODE1 = 0 (PD4 Output)
		OPA_CTLR1_OPA_EN1 |
		OPA_CTLR1_FB_EN1 | // Extra feedback resistor (SOMETIMES IT GOOD SOMETIMES IT BAD)
		//OPA_CTLR1_VBEN | // Toying with + input - it lowers the voltage for more headroom but makes everything rattier.
		//OPA_CTLR1_VBSEL |
		0;

	//CMP1 has option to turn on +/- 24mV

	// To fiddle:
	// Disable/enable FB_EN1.

	// Comparators:
	// CMP2 = OPA out // CMP2_REF -> TIM1_BKIN
	// CMP2 cannot output directly.

	OPA->CTLR2 = 
		OPA_CTRL2_HYEN1 |
		0;
	
	while(1)
	{
		//printf( "%d\n",  );
	}
}

