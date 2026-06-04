#include "ch32fun.h"
#include <stdio.h>

// use defines to make more meaningful names for our GPIO pins
#define PIN_1 PD0
#define PIN_K PD4
#define PIN_BOB PD6
#define PIN_KEVIN PC0

int main()
{
	SystemInit();

	funGpioInitAll(); // Enable GPIOs

	#define MP PC7

	GPIOC->CFGLR = 0x11111111;
	GPIOD->CFGLR = 0x44441144; // PD2..3 outputs

	//funPinMode( MP, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP ); // Set PIN_KEVIN to output

	while(1)
	{
		//funDigitalWrite( MP,     FUN_HIGH );
		GPIOC->OUTDR = 0xff;
		GPIOD->OUTDR = 0b1100;
		ADD_N_NOPS( 10 );
		//funDigitalWrite( MP,     FUN_LOW );
		GPIOC->OUTDR = 0x00;
		GPIOD->OUTDR = 0b0000;
		Delay_Ms( 1 );
	}
}
