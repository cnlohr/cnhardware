#include "ch32v003fun.h"
#include "rv003usb.h"
#include <stdio.h>
#include <limits.h>

#define ADC_DEPTH 800  
volatile uint16_t adc_buffer[ADC_DEPTH];

/*
 * initialize adc for DMA
 */
void adc_init( void )
{
	// ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
	// We choose main clock /64 with a 73 cycles sampling (+11 for actually converting)
	RCC->CFGR0 = (RCC->CFGR0 & ~(0x1F<<11)) | (0b11110 << 11);
	
	// Enable GPIOD and ADC
	RCC->APB2PCENR |=	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
						RCC_APB2Periph_ADC1;
	
	// PD4 is analog input chl 7
	//GPIOD->CFGLR &= ~(0xf<<(4*4));	// CNF = 00: Analog, MODE = 00: Input
	funGpioInitAll();
	funPinMode( PD4, GPIO_CFGLR_IN_ANALOG );


	// Reset the ADC to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
	
	// Set up four conversions on chl 7, 4, 3, 2
	ADC1->RSQR1 = (0) << 20;	// 1 chl in the sequence
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = (7<<(5*0));
	
	// set sampling time for chl 7
	// 0:7 => 3/9/15/30/43/57/73/241 cycles
	ADC1->SAMPTR2 = (6<<(3*7));

	// turn on ADC
	ADC1->CTLR2 |= ADC_ADON;
	
	// Reset calibration
	ADC1->CTLR2 |= ADC_RSTCAL;
	while(ADC1->CTLR2 & ADC_RSTCAL);
	
	// Calibrate
	ADC1->CTLR2 |= ADC_CAL;
	while(ADC1->CTLR2 & ADC_CAL);
	
	// Turn on DMA
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	
	//DMA1_Channel1 is for ADC
	DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
	DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
 //	DMA1_Channel1->CNTR  = ADC_DEPTH;
	DMA1_Channel1->CFGR  =
		DMA_M2M_Disable |		 
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		//DMA_Mode_Circular |
		DMA_DIR_PeripheralSRC;
	
	// Turn on DMA channel 1
	DMA1_Channel1->CFGR |= DMA_CFGR1_EN;
	
	// enable scanning
	ADC1->CTLR1 |= ADC_SCAN;
	
	// Enable continuous conversion and DMA
	ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL;
	
	// start conversion
	ADC1->CTLR2 |= ADC_SWSTART;
}

/*
 * turn on op-amp, select input pins
 */
void opamp_init( void )
{
	// turn on the op-amp
	EXTEN->EXTEN_CTR |= EXTEN_OPA_EN;
	
	// select op-amp pos pin: 0 = PA2, 1 = PD7
	EXTEN->EXTEN_CTR |= EXTEN_OPA_PSEL;

	// select op-amp neg pin: 0 = PA1, 1 = PD0
	EXTEN->EXTEN_CTR |= EXTEN_OPA_NSEL;
}

int startTime;

/*
 * entry
 */
int main()
{
	SystemInit();
	funGpioInitAll();

	// start serial @ default 115200bps
	Delay_Ms(100);
	printf("\r\r\n\nadc_dma_opamp example\n\r");

	// init adc
	printf("initializing adc...");
	adc_init();
	printf("done.\n\r");
	
	// init op-amp
	printf("initializing op-amp...\n\r");
	opamp_init();
	printf("done.\n\r");

	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	printf("Waiting for USB...\n\r");
	int last_cnt = 0;
	while(1)
	{
//		GPIOC->BSHR = 1<<1;	 // Turn on GPIOs
//		Delay_Ms( 100 );
//		GPIOC->BSHR = 1<<(1+16); // Turn off GPIODs
//		Delay_Ms( 100 );
//		printf( "%d\n", adc_buffer[0] );
		int cnt = DMA1_Channel1->CNTR;
		if( !cnt && last_cnt )
		{
			int endTime = SysTick->CNT;
			int rmin = INT_MAX;
			int rmax = 0;
			printf( "%d,", endTime -startTime );
			int i;
			int iib = adc_buffer[0] << 6;
			for( i = 0; i < ADC_DEPTH; i++ )
			{
				int b = adc_buffer[i];
				iib = iib - (iib>>6) + b;
				if( iib < rmin ) rmin = iib;
				if( iib > rmax ) rmax = iib;
			}
			printf( "%d,%d,", rmin, rmax );
			if( rmax - rmin < 10000 )
			{
				printf( "-1" );
			}
			else
			{
				int threshold = (rmax - rmin)/2  + rmin;
				iib = adc_buffer[0] << 6;
				for( i = 0; i < ADC_DEPTH; i++ )
				{
					int b = adc_buffer[i];
					iib = iib - (iib>>6) + b;
					if( iib < threshold )
					{
						break;
					}
				}
				if( i == ADC_DEPTH )
					printf( "-1" );
				else
				{
					// 800 conversions total, but,
					// For a single quanta,
					// 84 samples per conversion
					// 64 base divisor clock.
					// 84*64/48 = 112 us per clock tick.
					int us = i * 112;
					printf( "%d", us );
				}
			}
			printf( "\n" );
		}
		last_cnt = cnt;
	}
}




void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		// Keyboard (8 bytes)
		static int i;
		static int wasdown;
		static uint8_t tsakeyboard[8] = { 0x00 };

		i++;

		// Press a Key every half second or so.
		if( (i & 0xff) < 0x10 )
		//if( 0 )
		{
			tsakeyboard[2] = 1; // Button
			if( !wasdown )
			{
				//Trigger a run.
				startTime = SysTick->CNT;
				DMA1_Channel1->CNTR  = ADC_DEPTH;
 			}
			wasdown = 1;
		}
		else
		{
			tsakeyboard[2] = 0;
			wasdown = 0;
		}
		usb_send_data( tsakeyboard, 3, 0, sendtok );
	}
	else
	{
		// If it's a control transfer, empty it.
		usb_send_empty( sendtok );
	}
}


