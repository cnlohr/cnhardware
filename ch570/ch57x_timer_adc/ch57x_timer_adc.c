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


#define USE_UART_OUT

uint32_t count;

int last = 0;
void handle_debug_input( int numbytes, uint8_t * data )
{
	last = data[0];
	count += numbytes;
}

uint32_t timer_data[256];


void RunFunction() __HIGH_CODE;

#define PWMCLOCKDIV 4
// SPS = F_CPU/(PWMCLOCKDIV*255*oversample)

// Highest quality combination so far - 100MHz / (1 oversample*5clockdiv*255) = 78.431kSPS
// Good quality combination so far - 75MHz / (1 oversample*5clockdiv*255) = 78.824kSPS
// Actually amazing: 100MHz / (1 oversample*4clockdiv*255) = 98.039kSPS

void RunFunction()
{

	const uint32_t oversample = 1;
	const uint32_t targetsampletime = oversample*(255*R16_PWM_CLOCK_DIV); // in 1/60Mths of a second.

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

				//audio_samples[audiosamphead] = rvalue;
				//audiosamphead = (audiosamphead + 1) % ASAMPS;

				int err = (rvalue>>2) - remote_estimate;
				int value = 0;

				if( err > 127 ) err = 127;
				if( err < -127 ) err = -127;
				value = err;

				while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
#if 0
	static int ak;
				ak++;
				value = ak;
#endif
				R8_UART1_THR = value;
				//printf( "%d\n", value );
				remote_estimate += err;



				accumulated_dc += rvalue;
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




		// Steps disrupt the sampling. Minimize how often we do it.
		if( accumulated_dc > 10000000 && R8_PWM1_DATA > 0 ) { R8_PWM1_DATA--; accumulated_dc = 0; }
		if( accumulated_dc <-10000000 && R8_PWM1_DATA < 254 ) { R8_PWM1_DATA++; accumulated_dc = 0; }
		accumulated_dc = accumulated_dc - (accumulated_dc>>19);

		//if( R8_PWM1_DATA++ > 28 ) R8_PWM1_DATA = 1;
	}
}

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

	// UART Config
//	SetupUART(FUNCONF_UART_PRINTF_BAUD); // UART1 on rx,tx:PA8,PA9 at 115200 baud (PA2,PA3 on ch570/2)

	//PA10 TXD_7: TXD pin mapping of UART.
	funPinMode(PA10, GPIO_CFGLR_OUT_50Mhz_PP); //UART out.

	const uint32_t rate = 2000000;//921600;
	R16_UART1_DL = (const uint32_t)( ((10 * FUNCONF_SYSTEM_CORE_CLOCK / 8 / rate ) +5) /10 );
	R8_UART1_FCR = (2 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
	R8_UART1_LCR = RB_LCR_WORD_SZ | RB_LCR_STOP_BIT /*Extra stop bit*/;
	R8_UART1_IER = RB_IER_TXD_EN;
	R8_UART1_DIV = 1;


	R16_PIN_ALTERNATE_H = RB_UART_TXD;
#if 0
	while(1)
	{
		while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
		R8_UART1_THR = 'U';
		while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
		R8_UART1_THR = '\n';
	}
#endif

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
	R16_PWM_CLOCK_DIV = PWMCLOCKDIV;
    R8_PWM_OUT_EN |= (1 << (0)); // Enable output 0.
    R8_PWM_CONFIG &= ~(RB_PWM_CYC_MOD | RB_PWM_CYCLE_SEL);
    R8_PWM_CONFIG |= 0 << 2; // 0 = 8-bit, 3 << 2 = 5-bit (??? why not?), 2 << 2 = 6-bit, 1 << 2 = 7-bit.
    R8_PWM_CONFIG |= RB_PWM_CYCLE_SEL;
	R8_PWM1_DATA = 160; // Adjust 0 offset.


	RunFunction();

}
