#include "ch32fun.h"
#include <stdio.h>

#define LEDMASK 0b0000010000

int main()
{
	SystemInit();
	Delay_Ms( 1800 );

	funGpioInitAll(); // Enable GPIOs

	#define MP PC7

	GPIOC->CFGLR = 0x11111111;
	GPIOD->CFGLR = 0x44441144; // PD2..3 outputs

	//funPinMode( MP, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP ); // Set PIN_KEVIN to output

	while(1)
	{
		//funDigitalWrite( MP,     FUN_HIGH );
		GPIOC->OUTDR = LEDMASK;
		GPIOD->OUTDR = (LEDMASK>>8)<<2;
		ADD_N_NOPS( 15 );
		//funDigitalWrite( MP,     FUN_LOW );
		GPIOC->OUTDR = 0x00;
		GPIOD->OUTDR = 0b0000;
		ADD_N_NOPS( 50 );
	}
}
