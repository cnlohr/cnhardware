//
// 
//


#include "ch32fun.h"
#include <stdio.h>

int main()
{
	SystemInit();
	funGpioInitAll();

	// Enable GPIOC, GPIOD and TIM1
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC |
		RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO;

	funPinMode( PC5, GPIO_CFGLR_OUT_50Mhz_PP );	
	funDigitalWrite( PC5, 1 );

	funPinMode( PC6, GPIO_CFGLR_OUT_50Mhz_AF_OD );

	//TIM1_RM=01
	AFIO->PCFR1 |= AFIO_PCFR1_TIM1_REMAP_0;

	// Reset TIM1 to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

	// Prescaler 
	TIM1->PSC = 0x0000;

	// Auto Reload - sets period
	TIM1->ATRLR = 90;

	// Reload immediately
	TIM1->SWEVGR |= TIM_UG;
	
	// Enable CH1N output, positive pol
	TIM1->CCER |= TIM_CC1E | TIM_CC1P;

	// CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1;

	// Set the Capture Compare Register value to 50% initially
	TIM1->CH1CVR = 71;
	
	// Enable TIM1 outputs
	TIM1->BDTR |= TIM_MOE;
	
	// Enable TIM1
	TIM1->CTLR1 |= TIM_CEN;

	Delay_Ms( 100 );

//GPIO_CFGLR_IN_FLOAT, GPIO_CFGLR_OUT_50Mhz_OD
//	funPinMode( PC6, GPIO_CFGLR_OUT_50Mhz_PP );

	while(1)
	{
	
	}
}
