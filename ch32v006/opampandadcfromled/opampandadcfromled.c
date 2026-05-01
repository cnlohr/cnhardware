/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

// New Attempt: TIA mode.
// PC0 = GND = D+
// PA2 = reference pin, Driven low, but pulled up by vben/vbsel
// PA1 = D- (NSEL = 0)
// PA4 = unused
// VBEN = 0
// nsel = 0

#define ADCBUFSIZ 64
volatile uint16_t adc_buffer[ADCBUFSIZ];
int adctail;

/*
 * start conversion, wait and return result
 */
uint16_t adc_get( void )
{
	// start sw conversion (auto clears)
	ADC1->CTLR2 |= ADC_FLAG_STRT;
	
	// wait for conversion complete
	while(!(ADC1->STATR & ADC_EOC));
	
	// get result
	return ADC1->RDATAR;
}

uint32_t adc_oversample( void )
{
	uint32_t sum = 0;

	int i;
	for( i = 0; i < 5555; i++ )
	{
		// If CNTR == 128, 127 is OK
		// if CNTR == 127, 0 is OK
		int head;
		do
		{
			head = ((ADCBUFSIZ-1) - DMA1_Channel1->CNTR) & (ADCBUFSIZ-1);
		} while( head == adctail );
		uint16_t val = adc_buffer[adctail];
		adctail = (adctail+1)&(ADCBUFSIZ-1);
		sum += val;
	}
	return sum;
}


void adc_init( void )
{
	// ADCCLK = 24 MHz => RCC_ADCPRE divide by 2
	// ADC_CLK_MODE = 0 -- HB clock divides to make ADC clock
	// ADC_CLK_ADJ = 0 -- 1/2 duty cycle high level ADC clock
	RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_ADCPRE | RCC_CFGR0_ADC_CLK_MODE | RCC_CFGR0_ADC_CLK_ADJ)) | RCC_ADCPRE_DIV2;
	
	// Enable GPIOD and ADC
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;

	// Reset the ADC to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;



	
	// Turn on ADC module
	ADC1->CTLR2 |= ADC_ADON;

	// Set up single conversion on chl 7  ### is this necessary?
	ADC1->RSQR1 = (1<<20); // 1 channel
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 9;	// 0-9 for 8 ext inputs and two internals
	
	// set sampling time for chl 7
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*9));

	// No differences.
	ADC1->SAMPTR2 |= 3<<(3*9);	// 0:7 => 3.5/7.5/11.5/19.5/35.5/55.5/71.5/239.5 cycles

	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	
	//DMA1_Channel1 is for ADC
	DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
	DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
	DMA1_Channel1->CNTR  = ADCBUFSIZ;
	DMA1_Channel1->CFGR  =
		DMA_M2M_Disable |		 
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		DMA_Mode_Circular |
		DMA_DIR_PeripheralSRC |
		DMA_CFGR1_EN;

	// enable scanning
	ADC1->CTLR1 |= ADC_SCAN;

	// set External trigger event to SWSTART (for non-DMA mode)
	ADC1->CTLR2 = ADC1->CTLR2 & ~ADC_EXTSEL_SWSTART;
	
	// Enable continuous conversion and DMA
	ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL;
	
	// start conversion
	ADC1->CTLR2 |= ADC_SWSTART;
}

int main()
{
	SystemInit();

	funGpioInitAll();
	
	// LED power
	funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PC0, 0);

	// PSEL (Drive low, but internally slight pullup by VBEN/VBSEL
	//funPinMode( PA2, GPIO_CFGLR_OUT_10Mhz_PP );
	funPinMode( PA2, GPIO_CFGLR_OUT_10Mhz_PP );
	//funPinMode( PA2, GPIO_CFGLR_IN_PUPD );
	//funPinMode( PA2, GPIO_CFGLR_IN_FLOAT );
	funDigitalWrite( PA2, 0 );

	// NSEL (+)
	funPinMode( PA1, GPIO_CFGLR_IN_FLOAT );
	funDigitalWrite( PA1, 0 );

	// Unexposed PA4, to tie to PGADIF
	funPinMode( PA4, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PA4, 0 );

	OPA->OPA_KEY = 0x45670123;
	OPA->OPA_KEY = 0xCDEF89AB;

	// 400 1300 < No VBEN
	// 1400 4300 < VBEN

	//NOTE:!!! TRy with and without VBEN
	OPA->CTLR1 =
		//OPA_CTLR1_VBEN | // VBEN to pull + to VDD vs VBEN = 0 -> Directly connect + to that pin.
		//OPA_CTLR1_VBSEL |  //(No impact?)
		0 | // Connect PSEL1 to PA2
		0 | // Disconnect PSEL1.
		(0b000<<8) | // Connect NSEL1 to PA1
		(0b0<<4) | // PSEL = PA2
		//(0b011<<8) | // Disconnect NSEL1
		//OPA_CTLR1_PGADIF | // Connect PGADIF to PA4 (now VCC) vs GND (0) (If commented)
		OPA_CTLR1_FB_EN1 | // FB_EN1 = 1 (Enable feedback)
		OPA_CTLR1_OPA_EN1 | // Enable Op-Amp
		OPA_CTLR1_OPA_HS1 | // High speed mode.
		0b10<<1; // Disable output
		0;

	adc_init();

	while(1)
	{
		uint32_t ovs = adc_oversample();
		printf( "%d\n", (int)ovs );
	//	printf( "." );
	//	printf( "%d %d\n", DMA1_Channel1->CNTR, ADC1->RDATAR );
#if 0
		if( ovs < 5000 )
		{
			funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP );
			funDigitalWrite( PC0, 1);
			funPinMode( PA1, GPIO_CFGLR_OUT_10Mhz_PP );
			funDigitalWrite( PA1, 0 );
			Delay_Us(5000-ovs);
		}
		funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP );
		funDigitalWrite( PC0, 0);
		funPinMode( PA1, GPIO_CFGLR_IN_FLOAT );
		funDigitalWrite( PA1, 0 );
#endif
		//Delay_Ms();
	}
}

