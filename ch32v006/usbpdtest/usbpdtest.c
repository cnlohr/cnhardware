/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>
#include "pd_info.h"
#include "decodetable.h"


uint32_t count;

#define ADCBUFSIZ 1024
volatile uint32_t adc_buffer[ADCBUFSIZ/2];

static int PDPacketPlace;
static uint8_t PDPacket[128];

#define CRC_POLY 0x04C11DB7
#define CRC_INIT 0xffffffff // Determined experimentally.

static uint32_t current_crc = CRC_INIT;

void PDResetPacket( ) __attribute__((noinline));
void PDProcessPacket( ) __attribute__((noinline));
int PDAcceptByte( uint8_t by ) __attribute__((noinline));

void PDResetPacket( )
{
	PDPacketPlace = 0;
	current_crc = CRC_INIT; // Tricky: Using this value makes our final crc 0
}

void PDProcessPacket()
{
    uint32_t ret = 0;
    uint32_t j, bit;
	uint32_t initialrcrc = current_crc;
    current_crc = ~current_crc;
    for(int i=0;i<32;i++) {
        j = 31-i;
        bit = (current_crc>>i) & 1;
        ret |= bit<<j;
    }

	printf( "%08x %08x\n", initialrcrc, ret );
	int i;
	for( i = 0; i < PDPacketPlace; i++ )
	{
		printf( "%02x, ", PDPacket[i] );
	}
	printf( "\n" );
}

int PDAcceptByte( uint8_t by )
{
	if( PDPacketPlace >= sizeof( PDPacket ) ) return 1;
	PDPacket[PDPacketPlace++] = by;

	uint32_t newbit, newword;
	uint32_t rl_crc;
	for(int i=0; i<8; i++) {
		newbit = ((current_crc>>31) ^ ((by>>i)&1)) & 1;
		if(newbit) newword=CRC_POLY-1; else newword=0;
		rl_crc = (current_crc<<1) | newbit;
		current_crc = rl_crc ^ newword;
	}

	return 0;
}

void process( void ) __attribute__((noinline)) __attribute__( ( section( ".srodata" ) ) );

void process( void )
{
//#define COUNTDOWN
	int i;
#ifdef COUNTDOWN
	static int countdown = 100000;
#endif
	volatile uint32_t * dmacntr = &DMA1_Channel1->CNTR;

	static int observeTrigger = 0;
	static uint32_t observe[256];

	struct decodeState
	{
		uint8_t stage;    // range?
		uint8_t bitplace; // 0..5
		uint8_t word;     // 5 bits at a time.
		uint8_t outcode;
	};

	static uint8_t stateBits_s;
	uint8_t stateBits = stateBits_s;

	static struct decodeState state_s;
	struct decodeState state = state_s;

	static uint32_t adctail_s = 0;
	uint32_t adctail = adctail_s;

	volatile uint16_t * adcbufptr = (uint16_t*)adc_buffer;

	while(1)
	{
		uint32_t head = (((ADCBUFSIZ-1) - *dmacntr));

		//uint32_t outstanding = (((uint32_t)(head - adctail))&(ADCBUFSIZ-1)) >> 1U;
		uint32_t outstanding = (((uint32_t)(head - adctail))>>1) & (ADCBUFSIZ/2-1);

		// No outstanding data to process.
		if( !outstanding ) break;

		if( outstanding > 200 ) printf( "Underflow (%d %d)\n", head, adctail );


		while( outstanding-- )
		{

#ifdef COUNTDOWN
		if( countdown-- == 0 ) { printf( "%d", (head - adctail)&((ADCBUFSIZ>>1)-1) ); countdown = 100000; }
#endif
			uint16_t val = adcbufptr[adctail];
			adctail = (adctail+2)&(ADCBUFSIZ-1);

			int thisSample = val > 700;

			stateBits = bitDecodeLUT[stateBits | thisSample];

			if( stateBits & 1 )
			{

				// Emit!
				int bit = stateBits >> 6; // May contain failure info.
				stateBits &= 0x3e; // Clear out output bits.

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
							state.bitplace = 2;
							if( bit == 1 );
							else
							{
								state.stage = 1; // keep bitplace @ 2.
							}
						}
						else if( state.bitplace == 2 )
						{
							// expect a 0.
							if( bit == 0 ) state.bitplace = 1;
							else fail = 1;
						}
#if 0
						if( observeTrigger ) {
							printf("\n" );
							for( i = 0; i < observeTrigger; i++ )
							{
								printf( "%06x,", observe[i] );
							}
							observeTrigger = 0;
						}
#endif

						break;
					case 1: // In SOP, expect S1, S1, S1, S2.
					case 2: // In later SOP, already got S1 (may happen multiple times)
					case 3: // In data.
					case 4: // In data (Alt)
					{
						int nextword = state.word;
						int nextbit = state.bitplace;
						nextword = nextword | ( bit << nextbit );
						nextbit++;

						if( nextbit == 5 )
						{
							int kcode = pd_kcode_lut[ nextword ];

							switch( state.stage )
							{
							case 1:
								if( kcode == KCODE_SYNC1 ) state.stage = 2;
								else fail = 1;
								break;
							case 2:
								if( kcode == KCODE_SYNC1 ) state.stage = 2;
								else if( kcode == KCODE_SYNC2 )
								{
									state.stage = 3;
									PDResetPacket();
								}
								else fail = 1;
								break;
							case 3:
							case 4:
								if( kcode == KCODE_EOP )
								{
									PDProcessPacket();
								}
								else if( kcode >= 0x10 )
								{
									fail = 1;
								}
								else
								{
									if( state.stage == 3 )
									{
										state.outcode = kcode;
										state.stage = 4;
									}
									else
									{
										fail = PDAcceptByte( ( kcode << 4 ) | state.outcode );
										state.stage = 3;
									}
								}

								break;

							}
#if 0
							if( observeTrigger < 100 )
								observe[observeTrigger++] = nextword | (fail<<8) | (state.stage<<16); //( thisSample << 8 ) | stateBits;
#endif

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
	ADC1->RSQR1 = (1<<20); // 2 channels
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 3 | (2<<5);	// 0-9 for 8 ext inputs and two internals
	ADC1->SAMPTR1 = 0;
	ADC1->SAMPTR2 = (1<<(3*3)) | 2<<(3*2);	// 0:7 => 3.5/7.5/11.5/19.5/35.5/55.5/71.5/239.5 cycles


	// enable scanning
	ADC1->CTLR1 = ADC_SCAN | ADC_BUFEN;

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

