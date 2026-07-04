/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

int main()
{
	SystemInit();

	// Force HSE.
	

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1;

	RCC->CFGR0 |= RCC_ADCPRE_DIV8; // Divide ADC timer
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
	TIM1->CH4CVR = 4; // This is what we are controlling.

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
		for( i = 60; i < 92; i++ )
		{
			int j;
			for( j = 0; j < 16; j++ )
			{
				while(!(ADC1->STATR & ADC_EOC));
				vals[i] += ADC1->RDATAR;
				TIM1->CH4CVR = i;
			}
		}
		for( i = 60; i < 92; i++ )
		{
			printf( "%d ", (int)vals[i] );
		}
		printf( "\n" );
	}
}

