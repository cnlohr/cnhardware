/* 
	In progress.  Trying to jank an ADC out of a ch57x.

	// Mic (with pull-up) on pa3
	// RC on PC7->PC2
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

int audiosamphead;
int audiosamptail;
#define ASAMPS 256
int16_t  audio_samples[ASAMPS];

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	funPinMode( LED, GPIO_CFGLR_OUT_10Mhz_PP );

	funPinMode(PA7, GPIO_CFGLR_OUT_2Mhz_PP); //PWM1

	// Pull-up PA3
	R32_PA_PU |= 0b1100; // Extra pull-up on mic - why?
	R32_PA_PD_DRV |= 0b0100;
	// doesn't do anything?

	// Configure comparator to output to timer capture.
	R32_CMP_CTRL = 
		(0<<25) | // RB_APR_OUT_CMP = realtime output. -- what does this even do?
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
	// Or 100MHz...
	R16_PWM_CLOCK_DIV = 8;
	uint32_t targetsampletime = (255*R16_PWM_CLOCK_DIV); // in 1/60Mths of a second.
	// SPS = F_CPU/(R16_PWM_CLOCK_DIV*255)
    R8_PWM_OUT_EN |= (1 << (0)); // Enable output 0.
    R8_PWM_CONFIG &= ~(RB_PWM_CYC_MOD | RB_PWM_CYCLE_SEL);
    R8_PWM_CONFIG |= 0 << 2; // 0 = 8-bit, 3 << 2 = 5-bit (??? why not?), 2 << 2 = 6-bit, 1 << 2 = 7-bit.
    R8_PWM_CONFIG |= RB_PWM_CYCLE_SEL;
	R8_PWM1_DATA = 160; // Adjust 0 offset.

	uint32_t * rptr = timer_data;
	uint32_t * end = &timer_data[sizeof(timer_data)/sizeof(timer_data[0])];
	uint32_t place = 0;
	int32_t rvalue = 0;

	int remote_estimate = 0;

	int accumulated_dc = 0;

	while(1)
	{
		uint32_t * cnow = (void*) (R16_TMR_DMA_NOW | (((intptr_t)timer_data)&0xffff0000));
		if( cnow == end ) cnow = end - 1;
		while( rptr != cnow )
		{
			uint32_t v = *(rptr++);

			// For each edge.
			int negative = 0;
			if( v & 0x02000000 )
			{
				v = v & 0x1ffffff;
				negative = 1;
			}

			int tprog = v;

//			place += v;

			do
			{
				int leftover = place + tprog - targetsampletime;

				if( leftover > 0 )
				{
					// We bridge over.  Need to see how much data was left.
					int fillcur = targetsampletime - place;
					place += fillcur;
					tprog -= fillcur;
					if( negative )
						rvalue -= fillcur;
					else
						rvalue += fillcur;					
				}
				else
				{
					// No leftover.  Add remaining part.
					place += tprog;
					if( negative )
						rvalue -= tprog;
					else
						rvalue += tprog;
					break;
				}


				audio_samples[audiosamphead] = rvalue;
				accumulated_dc += rvalue;
				audiosamphead = (audiosamphead + 1) % ASAMPS;

				rvalue = 0;

				place -= targetsampletime;
			} while(1);



			if( rptr == end ) rptr = timer_data;
		}


		//printf( "%08x\n", R16_TMR_DMA_NOW );
		// Should print 00001310 020000a0 00001310 020000a0
//		printf( "%08x %08x %08x %08x @ %08x\n", (int)timer_data[0], (int)timer_data[1], (int)timer_data[2], (int)timer_data[3], R16_TMR_DMA_NOW );
		//printf( "%6d %5d %d\n", audio_samples[audiosamphead], place, audiosamphead );
	//	printf( "%x\n",  audio_samples[(audiosamphead-1+ASAMPS)&(ASAMPS-1)]+32767 );
		//printf( "." );
	//	_write( 0, "12345", 5 );

		do
		{
			uint32_t remain = ( audiosamphead - audiosamptail ) & ( ASAMPS - 1 );
			if( remain < 8 ) break;
			uint8_t senda[7];
			int i;
			for( i = 0; i < 7; i++ )
			{
				int samp = audio_samples[ (audiosamptail++) & (ASAMPS-1) ] >> 2;
				int err = samp - remote_estimate;
				if( err > 127 ) err = 127;
				if( err < -128 ) err = -128;
				remote_estimate += err;
				senda[i] = err;
			}
			_write( 1, (void*)senda, 7 );

/*			uint32_t remain = ( audiosamphead - audiosamptail ) & ( ASAMPS - 1 );
			if( remain < 4 ) break;
			uint16_t senda[3];
			senda[0] = audio_samples[ (audiosamptail++) & (ASAMPS-1) ];
			senda[1] = audio_samples[ (audiosamptail++) & (ASAMPS-1) ];
			senda[2] = audio_samples[ (audiosamptail++) & (ASAMPS-1) ];


			_write( 1, (void*)senda, 6 );*/


/*
			char da[7];
			int n = sprintf( da, "%d\n", senda[0] );
			_write(1,da,n);
			n = sprintf( da, "%d\n", senda[1] );
			_write(1,da,n);
			n = sprintf( da, "%d\n", senda[2] );
			_write(1,da,n);
*/
			//Delay_Us(10);
			//printf( "." );
//			_write(1, "1234567", 7 );
		} while( 1 );
		


		// Steps disrupt the sampling. Minimize how often we do it.
		if( accumulated_dc > 1000000 && R8_PWM1_DATA > 0 ) { R8_PWM1_DATA--; accumulated_dc = 0; }
		if( accumulated_dc <-1000000 && R8_PWM1_DATA < 254 ) { R8_PWM1_DATA++; accumulated_dc = 0; }
		accumulated_dc = accumulated_dc - (accumulated_dc>>11);

		//if( R8_PWM1_DATA++ > 28 ) R8_PWM1_DATA = 1;
	}
}
