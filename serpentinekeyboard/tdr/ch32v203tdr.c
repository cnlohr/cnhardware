/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

int main()
{
	RCC->APB1PCENR |= RCC_APB1Periph_PWR; // Enable LDO control

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1;

	// LDO to performance mode.
	EXTEN->EXTEN_CTR = 0xf50 | (1<<5);

// We will do our own clock tree initialization.
	SetupDebugPrintf();


	uint32_t tmp32 = RCC->CFGR0 & ~(0x03);
	RCC->CFGR0 = tmp32 | RCC_SW_HSI;

	// When in turbo mode...
	// 003d040a CFGR0
	//	PLLx18
	//  PB1 HCLK/2
	//  PLL/PLL
	//
	// 030b6d83 CTLR
	//  PLL on + ready
	//  CSS on
	//  HSE on and READY
	//  HSICAL sane
	//  HSITRIM sane
	//  HSI on and READY

	// ??? Why are we able to get 240MHz out of an 8MHz xtal?  Did they put the wrong part on?

	// Wait before overclocking.
	DelaySysTick(5000000);

	#define BASE_CFGR0 \
		RCC_HPRE_DIV1 | \
		RCC_PPRE2_DIV1 | \
		RCC_PPRE1_DIV1 | \
		RCC_PLLMULL10 | /* Overclock to 240MHz, max is around 255MHz */ \
		RCC_PLLSRC; /* HSE routes into to PLL*/ 
		//0b1011<<24; /*PLL2 Out*/ /* RCC_CFGR0_MCO_SYSCLK */

	RCC->CFGR0 = BASE_CFGR0;
	RCC->CTLR  = ((FUNCONF_HSITRIM) << 3) | RCC_HSION | RCC_HSEON | RCC_PLLON;

	while((RCC->CTLR & RCC_PLLRDY) == 0) {};                       	// Wait till PLL is ready
	tmp32 = RCC->CFGR0 & ~(0x03);							// clr the SW

	RCC->CFGR0 = tmp32 | RCC_SW_PLL;                       			// Select PLL as system clock source
	while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08); 	// Wait till PLL is used as system clock source

	// Test overclocking with `make clean all monitor | pv`
	if( 0 )
	{
		uint32_t term = SysTick->CNT;
		while(1)
		{
			while( TimeElapsed32( SysTick->CNT, term ) < 0 );
			printf( "1" );
			term += 1000000;
		}
	}

	// ADCCLK = 240MHz / 8 = 30MHz (Maximum 14MHz, works up to 60MHz)
	RCC->CFGR0 |= RCC_ADCPRE_DIV8;
	RCC->CTLR &= ~3; // HSI Off

	// Configure ADC first.
	// Note to self: DO NOT USE BUFFER
	ADC1->CTLR1 = ( 0 << 0);
	ADC1->CTLR2 = ADC_EXTTRIG | ADC_ADON | ( 0 << 17 ); // EXTSEL = T1CC1
	ADC1->SAMPTR2 = 0; // 1.5 cycle SAMPTR = 0, 1 = 7.5 samples, does not change edge

	// Prescaler (Actually run TIM1 @ 144MHz)
	TIM1->PSC = 0x0000;


	// Actual time is this+1
	// Must be divisible by ADC setup.
	TIM1->ATRLR = 511;

	TIM1->SWEVGR |= TIM_UG;
	
	TIM1->CCER |= TIM_CC1E | TIM_CC1P;
	TIM1->CCER |= TIM_CC4E | TIM_CC4P;
	TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1;
	TIM1->CHCTLR2 |= TIM_OC4M_2 | TIM_OC4M_1;

	TIM1->CH1CVR = 68; // CH1 triggers ADC.  Be careful where this is set.
	TIM1->CH4CVR = 4; // This is what we are controlling. (PWM output (is step function)

	TIM1->CTLR1 = 0;

	// Enable TIM1 outputs
	TIM1->BDTR |= TIM_MOE;
	TIM1->CTLR1 |= TIM_CEN;

	funPinMode( PA0, GPIO_CNF_IN_ANALOG );

	// PWM outputs.  We only actually use PA11, PA8 is diagnostic for the ADC trigger.
	funPinMode( PA8, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );
	funPinMode( PA9, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );
	funPinMode( PA10, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );
	funPinMode( PA11, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );

	int framestart = 60;
	int frame = framestart;
	int32_t vals[128];

	while(1)
	{
		memset( vals, 0, sizeof( vals ) );
		int i;
		(void)ADC1->RDATAR;
		int j;
		for( j = 0; j < 10 /* Oversample */; j++ )
		{
			for( i = 25; i < 92; i++ )
			{
				while(!(ADC1->STATR & ADC_EOC));
				vals[i] += ADC1->RDATAR;
				TIM1->CH4CVR = i;
			}
		}
		for( i = 30; i < 92; i++ )
		{
			printf( "%d ", (int)vals[i] );
		}
		printf( "\n" );
	}
}

