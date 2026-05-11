
//           FLASH:        3932 B        62 KB      6.19%
//             RAM:        2688 B         8 KB     32.81%

/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>
#include "pd_info.h"
#include "decodetable.h"

#define ADCBUFSIZ 128
volatile uint32_t adc_buffer[ADCBUFSIZ/2];

// Worded this way to keep the code clean but
// internally, GCC will register-allocate each
// one of these fields as a byte.
struct cnpd_decodeState_t
{
	uint8_t stage;    // range?
	uint8_t bitplace; // 0..5
	uint8_t word;     // 5 bits at a time.
	uint8_t outcode;
} __attribute__((packed));


struct cnpd_t
{
	// Organize struct so bytes are first, shorts are second, and
	// everything else in uint32_t are 4-byte-aligned.
	uint8_t stateBits;
	uint8_t  iopin;

	uint16_t reserved; //

	uint32_t adctail;
	struct cnpd_decodeState_t state;
	int ErrCtCRC;
	int ErrCtDecode;
	int ErrCtUnderflow;
	int payloadPlace;
	uint32_t current_crc;

	// Hardware-out.
	volatile GPIO_TypeDef * ioport;

	uint8_t payload[128];
} __attribute__((aligned(16)));


void PDResetPacket( struct cnpd_t * pd ) __attribute__((noinline));
void PDProcessPacket( struct cnpd_t * pd ) __attribute__((noinline));
int  PDAcceptByte( struct cnpd_t * pd, uint8_t by ) __attribute__((noinline));
void PDTick( struct cnpd_t * pd ) __attribute__((noinline)) __attribute__( ( section( ".srodata" ) ) );
void PDTransmit( struct cnpd_t * pd )  __attribute__((noinline));// __attribute__( ( section( ".srodata" ) ) );
void PDInit( struct cnpd_t * pd );

void PDResetPacket( struct cnpd_t *pd )
{
	pd->payloadPlace = 0;
	pd->current_crc = CRC_INIT; // Tricky: Using this value makes our final crc 0
}

void PDPrintPDO( uint32_t p )
{
	int type = p>>30;
	printf( "    %08x: ", (int)p );
	switch( type )
	{
		case 0b00: // Fixed (Tables 6.8-6.10)
		{
			int peak = (p>>20)&3;
			int V = (p>>10)&0x3ff;
			int A = (p>>0)&0x3ff;
			printf( " FIXED Peak: %d, %dmV %dmA\n", peak, V*50, A*10 );
			break;
		}
		case 0b10: // Variable (Table 6.12)
		case 0b01: // Battery (Table 6.11)
		{
			int Vmax = (p>>20)&0x3ff;
			int Vmin = (p>>10)&0x3ff;
			int A = (p>>0)&0x3ff;
			if( type == 0b01 )
				printf( " BATTERY :  %dmV - %dmV %dW\n", Vmin*50, Vmax*50, A/4 );
			else
				printf( " VARIABLE:  %dmV - %dmV %dmA\n", Vmin*50, Vmax*50, A*10 );
			break;
		}
		case 0b11: // APDO (Table 6.7)
		{
			int apdt = (p>>28)&3;
			static const char * APDOTYPE[] = { "PPS", "EPR AVS", "SPR AVS", "invalid" };
			if( apdt == 0 )
			{
				int Vmax = (p>>17)&0xff;
				int Vmin = (p>>8)&0xff;
				int I = (p)&0x7f;
				printf( " PPS: L:%d %dmV - %dmV %dmA\n", (int)((p>>27)&1), Vmin*100, Vmax*100, I*50 );
			}
			else
			{
				printf( " APDO Type: %s\n", APDOTYPE[apdt] );
			}
			break;
		}
			
	}
}

void PDProcessPacket( struct cnpd_t * pd)
{
	if( pd->current_crc != CHECKCRC_VALID )
	{
		pd->ErrCtCRC++;
		return;
	}

	// Ordering goes:
	//  Message Header (LSB)
	//     [ Spec Rev 7..6] [Port Data Role 5] [Message Type 4..0]
	//  Message Header (MSB)
	//     [ Extended 7 ] [Number of Objects 14..12] [Message ID 11..9] [Port Power Role 8]
	uint16_t * p16 = (uint16_t*)pd->payload;
	uint16_t header = (p16)[0];
	int type = header & 0x1f;
	int objects = (header>>12)&7;
	int extended = header >> 15;

	int handled = 0;
	printf( "Spec: %d / Data Role: %d / Type: %2d\n", (header>>6)&3, (header>>5)&1, type );
	printf( "Extended: %d / Objects: %d / Message ID: %d / PPR: %d\n", extended, objects, (header>>9)&7, (header>>8)&1 );

	if( extended || objects )
	{
		// ??? Extended is also this?
		// Data message: Table 6.5
		static const char * DataTypes[16] = { 
			"RSV", "SRCCAP", "REQ", "BIST",
			"SINKCAP", "BATSTAT", "ALERT", "COUNTRY",
			"ENTUSB", "EPRREQ", "EPRMOD", "SRCINFO",
			"REVISION", "RESV", "RESV", "VEND" };
		const char * typ = (type < 16)?DataTypes[type]:"RESV";
		printf( "DATA: %s\n", typ );
		switch( type )
		{
		case 0b00001: // SRCCAP
		case 0b00100: // SINKCAP
			for( int n = 0; n < objects; n++ ) PDPrintPDO( ( p16[n*2+1]) | ( p16[n*2+2] << 16) );
			handled = 1;
			break;
		}
	}
	else
	{
		// Control message: Table 6.4
	}

	if( !handled )
	{
		int i;
		for( i = 0; i < pd->payloadPlace; i++ )
		{
			printf( "%02x, ", pd->payload[i] );
		}
		printf( "\n" );
	}

	PDTransmit( pd );
}

int PDAcceptByte( struct cnpd_t * pd, uint8_t by )
{
	if( pd->payloadPlace >= sizeof( pd->payload ) ) return 1;
	pd->payload[pd->payloadPlace++] = by;
	uint32_t current_crc = pd->current_crc;
	current_crc = ( ( current_crc << 4 ) ) ^ crc_pd_table[((current_crc >> 28) ^ crc_pd_rev[by&0xf] )];
	current_crc = ( ( current_crc << 4 ) ) ^ crc_pd_table[((current_crc >> 28) ^ crc_pd_rev[by>>4 ] )];
	pd->current_crc = current_crc;
	return 0;
}

void PDTick( struct cnpd_t * pd )
{
//#define COUNTDOWN
#ifdef COUNTDOWN
	static int countdown = 100000;
#endif
	volatile uint32_t * dmacntr = &DMA1_Channel1->CNTR;

	uint8_t stateBits = pd->stateBits;
	struct cnpd_decodeState_t state = pd->state;
	uint32_t adctail = pd->adctail;

	volatile uint16_t * adcbufptr = (uint16_t*)adc_buffer;

	while(1)
	{
		uint32_t head = (((ADCBUFSIZ-1) - *dmacntr));

		//uint32_t outstanding = (((uint32_t)(head - adctail))&(ADCBUFSIZ-1)) >> 1U;
		uint32_t outstanding = (((uint32_t)(head - adctail))>>1) & (ADCBUFSIZ/2-1);

		// No outstanding data to process.
		if( !outstanding ) break;

		if( outstanding > (ADCBUFSIZ*3/4) ) pd->ErrCtUnderflow++;

		while( outstanding-- )
		{

#ifdef COUNTDOWN
		if( countdown-- == 0 ) { printf( "%d", (head - adctail)&((ADCBUFSIZ>>1)-1) ); countdown = 100000; }
#endif
			uint16_t val = adcbufptr[adctail];
			adctail = (adctail+2)&(ADCBUFSIZ-1);

			int thisSample = val > 700;

			stateBits = bitDecodeLUT[ (stateBits & 0x3e) | thisSample];

			if( stateBits & 1 )
			{

				// Emit!
				int bit = stateBits >> 6; // May contain failure info.

				if( bit > 1 )
				{
					// Fault!
					state = (struct cnpd_decodeState_t){ 0 };
				}
				else
				{
					int fail = 0;

					// Got a 0 or a 1 bit.
					if( state.stage == 0 )
					{
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

					}
					else
					{
						switch( state.stage )
						{
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
									if( kcode == KCODE_SYNC1 ) { }
									else if( kcode == KCODE_SYNC2 )
									{
										// Accepted start of packet.
										state.stage = 3;
										PDResetPacket( pd );
									}
									else fail = 1;
									break;
								case 3:
								case 4:
									if( kcode == KCODE_EOP )
									{
										PDProcessPacket( pd );
										goto skip_to_complete;
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
											fail = PDAcceptByte( pd, ( kcode << 4 ) | state.outcode );
											state.stage = 3;
										}
									}

									break;

								}

								nextword = 0;
								nextbit = 0;

							}
							state.word = nextword;
							state.bitplace = nextbit;
							break;
						}
						}
					}

					// Not else case since we may still fail.
					if( fail )
					{
						pd->ErrCtDecode++;
skip_to_complete:
						state = (struct cnpd_decodeState_t){ 0 };
					}
				}
			}
		}
	}
	pd->state = state;
	pd->stateBits = stateBits;
	pd->adctail = adctail;
	return;
}


void PDInit( struct cnpd_t * pd )
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

	// For now PD2
	pd->iopin = 2;
	pd->ioport = GPIOD;
}

void PDTransmit( struct cnpd_t * pd )
{
#if FUNCONF_SYSTICK_USE_HCLK != 1
#error FUNCONF_SYSTICK_USE_HCLK must be set to 1 in your funconfig.h
#endif

	uint32_t bitTime = FUNCONF_SYSTEM_CORE_CLOCK / 600000;
	uint32_t nextTime = SysTick->CNT + bitTime;

	volatile GPIO_TypeDef * port = pd->ioport;
	uint32_t tmp = 0;
	int iopincfgshift = pd->iopin*4;
	uint32_t cfgmaskclear = ~(0xf<<iopincfgshift);
	uint32_t cfgmaskfloat = 0b1001<<(iopincfgshift);
	uint32_t cfgmaskdrive = 0b0001<<(iopincfgshift);
	uint32_t pon = 1<<pd->iopin;

	int i;
	for( i = 0; i < 16; i++ )
	{
		while( ((int)( SysTick->CNT - nextTime )) < 0 );
		tmp = (port->CFGLR & cfgmaskclear) | cfgmaskdrive;
		asm volatile( "" : : : "memory" ); // Force barrier.
		port->BCR = pon; // OFF
		port->CFGLR = tmp; // DRIVE DOWN
		nextTime += bitTime;
		while( ((int)( SysTick->CNT - nextTime )) < 0 );

		//tmp = (port->CFGLR & cfgmaskclear) | (i<<iopincfgshift);
		//port->CFGLR = tmp; // Release

		tmp = (port->CFGLR & cfgmaskclear) | cfgmaskfloat;
		asm volatile( "" : : : "memory" ); // Force barrier.
		port->BSHR = pon; // ON
		port->CFGLR = tmp; // Release
		nextTime += bitTime;
	}
	funPinMode( PC4, GPIO_CFGLR_IN_FLOAT ); // AIN2
	funPinMode( PD2, GPIO_CFGLR_IN_FLOAT ); // AIN3
}

int main()
{
	SystemInit();

	funGpioInitAll();
	printf( "Start\n" );
	
	//funPinMode( PC4, GPIO_CFGLR_IN_ANALOG ); // AIN2
	//funPinMode( PD2, GPIO_CFGLR_IN_ANALOG ); // AIN3
	//funPinMode( PC4, GPIO_CFGLR_IN_FLOAT ); // AIN2
	//funPinMode( PD2, GPIO_CFGLR_IN_FLOAT ); // AIN3

	funPinMode( PC4, GPIO_CFGLR_IN_FLOAT ); // AIN2
	funPinMode( PD2, GPIO_CFGLR_IN_FLOAT ); // AIN3
	funDigitalWrite( PC4, 0 );
	funDigitalWrite( PD2, 0 );

	struct cnpd_t pd;

	PDInit( &pd );

	while(1)
	{
		PDTick( &pd );
	}
}

