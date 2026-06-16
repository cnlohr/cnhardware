#include "ch32fun.h"
#include <stdio.h>

#define LEDMASK 512

int main()
{
	SystemInit();
	Delay_Ms( 1800 );
	funGpioInitAll(); // Enable GPIOs
	GPIOC->CFGLR = 0x11111111;
	GPIOD->CFGLR = 0x44441144; // PD2..3 outputs
	while(1)
	{
		GPIOC->OUTDR = LEDMASK;
		GPIOD->OUTDR = (LEDMASK>>8)<<2;
		ADD_N_NOPS( 15 );
		GPIOC->OUTDR = 0x00;
		GPIOD->OUTDR = 0b0000;
		ADD_N_NOPS( 30 );
	}
}
