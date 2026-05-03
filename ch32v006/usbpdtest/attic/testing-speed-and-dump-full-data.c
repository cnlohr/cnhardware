/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

#define ADCBUFSIZ 128
volatile uint32_t adc_buffer[ADCBUFSIZ/2];

void process( void ) __attribute__((noinline)) __attribute__( ( section( ".srodata" ) ) );

void process( void )
{
//#define COUNTDOWN
	int i;
#ifdef COUNTDOWN
	static int countdown = 100000;
#endif
	static int trigger = 0;
	static int adctail = 0;
	static uint16_t observe[256];
	volatile uint32_t * dmacntr = &DMA1_Channel1->CNTR;
	while(1)
	{
		int head;
		head = (((ADCBUFSIZ-1) - *dmacntr) & (ADCBUFSIZ-1))>>1;
		if( head == adctail ) return;
#ifdef COUNTDOWN
		if( countdown-- == 0 ) { printf( "%d", (head - adctail)&((ADCBUFSIZ>>1)-1) ); countdown = 100000; }
#endif
		uint32_t val = adc_buffer[adctail];
		adctail = (adctail+1)&(ADCBUFSIZ/2-1);
		int cc1 = (int)(val & 0xffff);
		int cc2 = (int)(val >> 16);
		if( trigger == 0 )
		{
			if( cc2 < 1000 )
			{
				printf( "\n" );
				trigger = 1;
			}
		}
		else if( trigger <= 256 )
		{
			observe[trigger-1] = cc2;
			trigger++;
		}
		else if( trigger == 257 )
		{
			int i;
			trigger = 0;
			for( i = 0; i < 256; i++ )
			{
				printf( "%d,", observe[i] );
			}
			printf( "\n" );
		}

	}
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

