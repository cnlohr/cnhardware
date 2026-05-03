/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>
#include "decodetable.h"

uint32_t count;

#define ADCBUFSIZ 128
volatile uint16_t adc_buffer[ADCBUFSIZ];

void process( void ) __attribute__((noinline)) __attribute__( ( section( ".srodata" ) ) );

void process( void )
{
#define COUNTDOWN
	int i;
#ifdef COUNTDOWN
	static int countdown = 100000;
#endif
	static int observeTrigger = 0;
	static uint32_t observe[256];

	struct decodeState
	{
		uint8_t stage;    // range?
		uint8_t bitplace;
		uint8_t word;     // 5 bits at a time.
	};

	static uint8_t stateBits_s;
	uint8_t stateBits = stateBits_s;

	static struct decodeState state_s;
	struct decodeState state = state_s;

	static uint32_t adctail_s = 0;
	uint32_t adctail = adctail_s;

	while(1)
	{
		// Tricky, may be at end end.  Don't use without masking.
		uint32_t head = (((ADCBUFSIZ) - DMA1_Channel1->CNTR)); 
		uint32_t outstanding = (((uint32_t)(head - adctail))) & (ADCBUFSIZ-1);

		// No outstanding data to process.
		if( !outstanding ) break;

		if( outstanding > 64 ) printf( "Underflow (%d %d)\n", head, adctail );


		while( outstanding-- )
		{
			uint16_t val = adc_buffer[adctail];
			adctail = (adctail+1)&(ADCBUFSIZ-1);
#ifdef COUNTDOWN
		if( countdown-- == 0 ) { printf( "%d", outstanding ); countdown = 100000; }
#endif
continue;

			static int logging = 0;
			if( val < 700 ) logging = 1;
			if( logging )
			{
					observe[observeTrigger++] = val;

					if( observeTrigger == 100 )
					{
						printf("\n" );
						for( i = 0; i < 100; i++ )
						{
							printf( "%d,", observe[i] );
						}
						observeTrigger = 0;
						logging = 0;
					}
			}

			continue;
			int thisBit = val > 700;

			stateBits = bitDecodeLUT[stateBits | thisBit];


			if( stateBits & 1 )
			{

				// Emit!
				int bit = stateBits >> 6; // May contain failure info.

				stateBits &= 0x3e; // Clear out output bits.

#if 0
				if( bit > 1 )
				{
					// Fault!
					state = (struct decodeState){ 0 };
				}
				else
				{
					int fail = 0;

					// Got a 0 or a 1 bit.
					switch( state.stage )
					{
					case 0: // Preamble, expect 1
						if( state.bitplace == 0 )
						{
							state.bitplace = bit + 1; // 1 or 2.
						}
						else if( state.bitplace == 1 )
						{
							if( bit == 1 ) state.bitplace = 2;
							else state.stage = 2; // keep bitplace @ 2.
						}
						else if( state.bitplace == 2 )
						{
							// expect a 0.
							if( bit == 0 ) state.bitplace = 1;
							else fail = 1;
						}
						break;
					case 1: // In SOP, expect S1, S1, S1, S2.
					case 2: // In later SOP, already got S1 (may happen multiple times)
					case 3: // In data.
					{
						int nextword = state.word;
						nextword = (nextword<<1) | bit;
						int nextbit = state.bitplace;
						nextbit++;
						if( nextbit == 5 )
						{

							switch( state.stage )
							{
							case 1:
							case 2:
							case 3:
							}
							nextword = 0;
							nextbit = 0;

						}
						state.word = nextword;
						state.bitplace = nextbit;
						break;
					}
					}

					// Not else case since we may still fail.
					if( fail )
					{
						state = (struct decodeState){ 0 };
					}

				}
#endif
			}
		}
	}
	state_s = state;
	stateBits_s = stateBits;
	adctail_s = adctail;
	return;
}


void adc_init( void )
{
	// Enable GPIOD and ADC
	RCC->APB2PCENR |= RCC_APB2Periph_ADC1;
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

	//DMA1_Channel1 is for ADC
	DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
	DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
	DMA1_Channel1->CNTR  = ADCBUFSIZ; // dual channel mode
	DMA1_Channel1->CFGR  =
		DMA_M2M_Disable |		 
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		DMA_Mode_Circular |
		DMA_DIR_PeripheralSRC |
		DMA_CFGR1_EN;


	// ADCCLK = 48 MHz => RCC_ADCPRE divide by 2
	// RCC_CFGR0_ADC_CLK_MODE = 0 -- HB clock divides to make ADC clock -- vs 1 = No ADC Division.
	// RCC_CFGR0_ADC_CLK_ADJ = 0 -- 1/2 duty cycle high level ADC clock
	RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_ADCPRE | RCC_CFGR0_ADC_CLK_MODE | RCC_HPRE)) | RCC_CFGR0_ADC_CLK_MODE;

	// Reset the ADC to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
	
	// Turn on ADC module
	ADC1->CTLR2 |= ADC_ADON;

	// Set up single conversion on chl 7  ### is this necessary?
	ADC1->RSQR1 = (0<<20); // 1 channels
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 3; //3 | (2<<5);	// 0-9 for 8 ext inputs and two internals
	ADC1->SAMPTR1 = 0;
	ADC1->SAMPTR2 = (4<<(3*3)) | 2<<(3*2);	// 0:7 => 3.5/7.5/11.5/19.5/35.5/55.5/71.5/239.5 cycles


	// enable scanning
	ADC1->CTLR1 = ADC_SCAN;// | ADC_BUFEN;

	// set External trigger event to SWSTART (for non-DMA mode)
	ADC1->CTLR2 = ADC1->CTLR2 & ~ADC_EXTSEL_SWSTART;
	
	// Enable continuous conversion and DMA
	ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL | ADC_SWSTART;
}

int main()
{
	SystemInit();

	funGpioInitAll();
	printf( "Start\n" );
	
	//funPinMode( PC4, GPIO_CFGLR_IN_ANALOG ); // AIN2
	//funPinMode( PD2, GPIO_CFGLR_IN_ANALOG ); // AIN3
	funPinMode( PC4, GPIO_CFGLR_IN_FLOAT ); // AIN2
	funPinMode( PD2, GPIO_CFGLR_IN_FLOAT ); // AIN3

	adc_init();

	while(1)
	{
		process();
	}
}

