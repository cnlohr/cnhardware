#include "ch32fun.h"
#include <stdio.h>

//int main() __attribute__((section(".srodata")));

int main()
{
	SystemInit();

	Delay_Ms(2500); // Allow a window of time to program.

	RCC->AHBPCENR = RCC_AHBPeriph_SRAM;
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO;

	// Generally best to pull up, doesn't use any extra power.
//	GPIOD->CFGLR = (GPIO_CFGLR_IN_PUPD)<<(4*1);
	GPIOD->CFGLR = (GPIO_CFGLR_IN_FLOAT)<<(4*1);

// PUPD draws more power.
//	funPinMode( PD1, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PD1, 1 );

	RCC->APB2PCENR |= RCC_AFIOEN;

	// assign pin 1 interrupt from portD (0b11) to EXTI
	AFIO->EXTICR |= (uint32_t)(0b11 << (1*2));

	// enable line2 interrupt event
	EXTI->EVENR |= EXTI_Line1;
	EXTI->FTENR |= EXTI_Line1;

	// select standby on power-down
	PWR->CTLR = PWR_CTLR_PDDS;

	// peripheral interrupt controller send to deep sleep
	PFIC->SCTLR |= (1 << 2);

#define DMTEST ((volatile uint32_t*)0xe0000124) // Reads as 0x00000000 if debugger is attached.
	printf( "%08x\n", *DMTEST );
//*DMSTATUS_SENTINEL = 0xffffffff;

//	printf( "%08x\n", *DMSTATUS_SENTINEL );
//	#define DMSTATUS_SENTINEL ((volatile uint32_t*)0xe00000fc) // Reads as 0x00000000 if debugger is attached.



	while(1)
	{
		__WFE();
		Delay_Ms(1000);
	}
}
