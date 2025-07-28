```c
// ADC test on ch570.

#include "ch32fun.h"
#include <stdio.h>

// VERF = 400mV and PA7 - CMP_VERF
// Timer Capture comes from Input (not that it matters) + Enable.
//
// Falling edge generates interrupt. (Disable interrupt for now)
#define CMP_CONFIG 0b001111111 | (2 << 10)

int takeReading( void ) __attribute__((section(".srodata")));
int takeReading( void ) 
{
	uint32_t r = SysTick->CNT;
	funPinMode( PA7, GPIO_ModeIN_Floating );
	// Time how long it takes to float up to 400mV.
	while(!(*((vu8*)0x40001057)));
	r = SysTick->CNT - r;
	funPinMode( PA7, GPIO_ModeOut_PP_5mA );
	return r;
}

int fullReading( void ) __attribute__((section(".srodata")));
int fullReading( void )
{
	// Get rid of any bias.  (This is required)
	takeReading();

	// Oversample like crazy.
	int i;
	uint32_t tot = 0;
	for( i = 0; i < 256; i++ )
	{
		tot += takeReading();
	}
	return tot>>4;
}

int main()
{
	int i;
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	R32_CMP_CTRL = CMP_CONFIG;

	while(1)
	{
		int r = fullReading();
		printf( "%d\n", r );
	}
}
```
