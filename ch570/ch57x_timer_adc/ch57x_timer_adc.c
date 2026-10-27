/* 
	In progress.  Trying to jank an ADC out of a ch57x.
*/




#include "ch32fun.h"
#include <stdio.h>

#ifdef CH570_CH572
#define LED PA9
#else
#define LED PA8
#endif

uint32_t count;

int last = 0;
void handle_debug_input( int numbytes, uint8_t * data )
{
	last = data[0];
	count += numbytes;
}

uint32_t timer_data[256];

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	funPinMode( LED, GPIO_CFGLR_OUT_10Mhz_PP );

	funPinMode(PA7, GPIO_CFGLR_OUT_2Mhz_PP); //PWM1

	R32_PA_PU |= 0b1000;
	R32_PA_PD_DRV |= 0b0100;

	// Configure comparator to output to timer capture.
	R32_CMP_CTRL = 
		(1<<25) | // RB_APR_OUT_CMP = realtime output.
		(0) |     // RB_CMP_SW = 0 = PA2/PA3.
		(1<<1) |  // RB_CMP_CAP = 1: Input comes from the comparator;
		(1<<0);   // RB_CMP_EN = 1.

	// Setup 26-bit capture timer - default on PA7.
	R8_TMR_CTRL_MOD = 
		(0b01<<6) | // RB_TMR_CAP_EDGE = Capture the time between any edge changes;
		0 |         // RB_TMR_CAP_COUNT = Capture mode.
		(1<<2) | 	// RB_TMR_COUNT_EN = 1
		(1<<1) |	// RB_TMR_ALL_CLEAR = 1
		(1<<0); 	// RB_TMR_MODE_IN = 1 (Input mode)
	R8_TMR_CTRL_MOD &= ~(1<<1); // Clear R8_TMR_CTRL_MOD.
	R16_TMR_DMA_BEG = (uintptr_t)&timer_data[0];
	R16_TMR_DMA_END = (uintptr_t)&timer_data[sizeof(timer_data)/sizeof(timer_data[0])]; // END is not inclusive.

	R8_TMR_CTRL_DMA =
		(1<<2) | // RB_TMR_DMA_LOOP
		(1<<0);  // RB_TMR_DMA_ENABLE


	// Base is 952.4 @ 5-bit (31 period) = 60MHz base clock.
	// Output at 1.9047619kHz with this setup.
	uint32_t targetsampletime = 31496; // 
	R16_PWM_CLOCK_DIV = 500;
    R8_PWM_OUT_EN |= (1 << (0));
    R8_PWM_CONFIG &= ~(RB_PWM_CYC_MOD | RB_PWM_CYCLE_SEL);
    R8_PWM_CONFIG |= 3 << 2; // 0 = 8-bit, 3 << 2 = 5-bit, 2 << 2 = 6-bit, 1 << 2 = 7-bit.
    R8_PWM_CONFIG |= RB_PWM_CYCLE_SEL;
	R8_PWM1_DATA = 31;

	uint32_t * rptr = timer_data;
	uint32_t * end = &timer_data[sizeof(timer_data)/sizeof(timer_data[0])];
	uint32_t pre = 0;
	uint32_t last = 0;

	while(1)
	{
		uint32_t * cnow = (void*) (R16_TMR_DMA_NOW | (((intptr_t)timer_data)&0xffff0000));
		if( cnow == end ) cnow = end - 1;
		while( rptr != cnow )
		{
			uint32_t v = *(rptr++);

			if( v & 0x02000000 )
			{
				v = v & 0x1ffffff;

				uint32_t sampdt = (v + pre);
				

				last = v;
			}
			else
			{
				uint32_t dt = v - last;

				if( dt < 

				last = v;
			}

			if( rptr == end ) rptr = timer_data;
		}
		//printf( "%08x\n", R16_TMR_DMA_NOW );
		// Should print 00001310 020000a0 00001310 020000a0
		printf( "%08x %08x %08x %08x @ %08x\n", (int)timer_data[0], (int)timer_data[1], (int)timer_data[2], (int)timer_data[3], R16_TMR_DMA_NOW );
		Delay_Ms(1);

		

		//if( R8_PWM1_DATA++ > 28 ) R8_PWM1_DATA = 1;
	}
}
