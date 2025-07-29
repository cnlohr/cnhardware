// ADC test on ch570.

#include "ch32fun.h"
#include <stdio.h>

// VERF = 400mV and PA7 - CMP_VERF
// Timer Capture comes from Input (not that it matters) + Enable.
//
// Falling edge generates interrupt. (Disable interrupt for now)
#define CMP_CONFIG ( 0x7 << 4 ) | 0b1111
// | (2 << 10)

int takeReading( void ) __attribute__((section(".srodata")));
int takeReading( void ) 
{
	funPinMode( PA7, GPIO_ModeIN_Floating );
	// Time how long it takes to float up to 400mV.
	//while(!(*((vu8*)0x40001057)));
	volatile uint8_t * vcheck = (uint8_t*)0x40001057;

	int elapsed;
	asm volatile("\n\
		li a3, 0x00010000\n\
		lw a4, 0(%[systickcnt])\n\
		li a2, 0\n\
.option   push;\n\
.option   norvc;\n\
rchk:\n\
		bnez a2, rcdone\n\
		lbu a2, 0(%[vcheck])\n\
		bnez a2, rcdone\n\
			addi a3, a3, -1\n\
		lbu a2, 0(%[vcheck])\n\
		bnez a2, rcdone\n\
			bnez a3, rchk\n\
		lbu a2, 0(%[vcheck])\n\
		bnez a2, rcdone\n\
		lbu a2, 0(%[vcheck])\n\
		j rchk /* ???? Why can't I do this the other way around */ \n\
.option   pop;\n\
rcto: \n\
		li %[r], -1\n\
		j finish\n\
rcdone:\n\
		lw a3, 0(%[systickcnt])\n\
		sub %[r], a3, a4\n\
finish:\n\
		" : [r]"=r"(elapsed) : [vcheck]"r"(vcheck), [systickcnt]"r"(&SysTick->CNT) : "a2", "a3", "a4"  );
	funPinMode( PA7, GPIO_ModeOut_PP_5mA );
	return elapsed;
}

int fullReading( void ) __attribute__((section(".srodata")));
int fullReading( void )
{
	// Get rid of any bias.  (This is required)
	takeReading();

	// Oversample like crazy.
	int i;
	uint32_t tot = 0;
	for( i = 0; i < 1; i++ )
	{
		tot += takeReading();
	}
	return tot;
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

