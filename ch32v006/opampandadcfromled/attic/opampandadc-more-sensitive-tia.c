/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

// Connecting:
//   LED - to PC0
//   LED + to PA1


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
	for( i = 0; i < 4096; i++ )
	{
		// start sw conversion (auto clears)
		ADC1->CTLR2 |= ADC_FLAG_STRT;
		
		// wait for conversion complete
		while(!(ADC1->STATR & ADC_EOC));
		
		// get result
		sum += ADC1->RDATAR;
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
	ADC1->RSQR1 = 0;
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 9;	// 0-9 for 8 ext inputs and two internals
	
	// set sampling time for chl 7
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*9));
	ADC1->SAMPTR2 |= 7<<(3*9);	// 0:7 => 3.5/7.5/11.5/19.5/35.5/55.5/71.5/239.5 cycles
		
	// set External trigger event to SWSTART
	ADC1->CTLR2 = ADC1->CTLR2 & ~ADC_EXTSEL_SWSTART;

}

int main()
{
	SystemInit();

	funGpioInitAll();
	
	// LED power
	funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PC0, 1 );

	// PSEL (Drive low)
	funPinMode( PA2, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PA2, 0 );

	// NSEL (+)
	funPinMode( PA1, GPIO_CFGLR_IN_FLOAT );
	funDigitalWrite( PA1, 0 );


	OPA->OPA_KEY = 0x45670123;
	OPA->OPA_KEY = 0xCDEF89AB;

	OPA->CTLR1 =
		OPA_CTLR1_VBEN | // VBEN to pull + to VDD.
		OPA_CTLR1_VBSEL |
		//0 | // VBEN = 0 -> Directly connect + to that pin.
		0 | // Connect PSEL1 to PA2
		0 | // Disconnect PSEL1.
		(0b000<<8) | // Connect NSEL1 to PA1
		//(0b111<<8) | // Disconnect NSEL1
		//OPA_CTLR1_PGADIF | // Connect PGADIF to PA4 (now VCC) vs GND (0) (If commented)
		OPA_CTLR1_FB_EN1 | // FB_EN1 = 1 (Enable feedback)
		OPA_CTLR1_OPA_EN1 | // Enable Op-Amp
		0;
	printf( "STARTING\n" );

	adc_init();

	printf( "INIT\n" );

	while(1)
	{
		printf( "%d\n", adc_oversample() );
	}
}

