// ADC test on ch570...
// 
// Methodology:
//  Connect a 0.1uF capcaitor between PA7 and GND
//  Connect a 5.1k resistor between PA7 and the voltage you want to measure.
//  WARNING: this will inject a little bit of noise on your signal.
//
//  Increasing resistance will improve reading quality, and reduce ripple, but take longer.
//  Increasing capacitance will reduce ripple, take more power and take longer.
//
// NOTE: the measurement is EXTREMELY sensitive to the capacitance.
//
// NOTE: Most capacitors in the 0.1-10uF range are more than their listed value.
// You should calibrate.
// NOTE: You may use PA4, but will need to change the code.

#include "ch32fun.h"
#include <stdio.h>

#define OVERSAMPLE  8
#define CAPACITANCE 0.0000001167
#define RESISTANCE  5100.0
#define VREF 0.1


// VERF = 100mV and PA7 - CMP_VERF
// Timer Capture comes from Input (not that it matters) + Enable.
//
// Rising edge generates interrupt. (Disable interrupt for now)
// After a lot of testing 100mV seems best.

#define CMP_CONFIG ( 0x2 << 4 ) | 0b1111 | (0b00<<10)

volatile unsigned switch_time;

// Because we are only operating on pinned register-variables, we can
// just straight up do a naked isr.
void CMP_IRQHandler(void) __attribute__((interrupt)) __attribute__((section(".srodata")));
void CMP_IRQHandler(void)
{
	switch_time = SysTick->CNT;
	R32_CMP_CTRL = CMP_CONFIG; // Disable ISR
}

void SysTick_Handler(void) __attribute__((interrupt))  __attribute__((section(".srodata")));
void SysTick_Handler(void)
{
	// Clear the trigger state for the next IRQ
	SysTick->SR = 0x00000000;
}

void SmartDelayOrISR( int n )  __attribute__((section(".srodata")));
void SmartDelayOrISR( int n ) 
{
#if !FUNCONF_SYSTICK_USE_HCLK
	#error This function requires FUNCONF_SYSTICK_USE_HCLK
#endif
	if( n < 220 )
	{
		uint32_t targend = SysTick->CNTL + n;
		while( ((int32_t)( SysTick->CNTL - targend )) < 0 );
	}
	else
	{
		SysTick->CMP = SysTick->CNT + n;
		SysTick->CTLR = 0x07; // Enable Interrupt
		__WFI();
		SysTick->CTLR = 0x05; // Disable Interrupt
	}
}

int takeReading( void ) __attribute__((section(".srodata")));

int takeReading( void ) 
{

	volatile uint32_t start = SysTick->CNT;
	switch_time = start;

	// Enable ISR
	R32_CMP_CTRL = CMP_CONFIG | (1<<8);

	funPinMode( PA7, GPIO_ModeIN_Floating );
	// Time how long it takes to float up to 400mV.

	SmartDelayOrISR( 100000 );

	R32_CMP_CTRL = CMP_CONFIG; // Disable ISR

	funPinMode( PA7, GPIO_ModeOut_PP_20mA );

	return switch_time - start;
}

int fullReading( void ) __attribute__((section(".srodata")));
int fullReading( void )
{
	// Oversample like crazy.
	int i;
	takeReading();
	uint32_t tot = 0;
	for( i = 0; i < OVERSAMPLE; i++ )
	{
		// Need 1us to reset ADC between samples.
		Delay_Us(1);
		tot += takeReading();
	}
	return tot;
}


void setupPA7ForADCRead()
{
	funPinMode( PA7, GPIO_ModeOut_PP_5mA );
	NVIC_EnableIRQ(ADC_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
	__enable_irq();
}

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx


	setupPA7ForADCRead();

	while(1)
	{
		uint32_t start = SysTick->CNT;
		int r = fullReading();
		uint32_t end = SysTick->CNT;
		r -= 8; // There is a delay before the release of the pin of about 8 cycles

		// tau = R * C
		// Vcomp = Vmeas(1 - 2.718 ^ (-t/tau))
		// Vmeas = Vcomp / (1 - 2.718 ^ (-t/tau))
		//
		// For example:
		//t = ticks / 60000000
		//tau = 5.1k * 0.1uF
		//Vcomp = 0.1V
		//Vmeas = Vcomp / (1 - e ^ (-t/tau))
		//
		//Vcomp = 0.1
		//t = 533 / 60000000
		//tau = 0.0051
		//-t/tau = -0.00174183
		//e^that = 0.998259686

		//A complete computation is = 0.1/(1-EXP(-ticks/60000000/(5100*0.0000001)))
		// Which oddly works out to almost exactly: = 3060/ticks+0.05
		// 3060=60000000*(5100*0.0000001)*0.1
		//
		// 3060000/ticks + 50 in millivolts.
		#define COEFFICIENT (const uint32_t)(FUNCONF_SYSTEM_CORE_CLOCK*(RESISTANCE*CAPACITANCE)*VREF*1000*OVERSAMPLE)

		int vtot = COEFFICIENT/r + 100; //100mV offset (vComp)
		printf( "%d %d %d\n", r, vtot, end-start );
	}
}


#if 0
// An example of doing the ADC read without ISRs
int takeReadingNoISR( void ) __attribute__((section(".srodata")));
int takeReadingNoISR( void )
{
	switch_time = SysTick->CNT;
	volatile uint32_t start = switch_time;

	// Enable ISR
	R32_CMP_CTRL = CMP_CONFIG;

	funPinMode( PA7, GPIO_ModeIN_Floating );
	// Time how long it takes to float up to 400mV.

	volatile uint8_t * v = (((vu8*)0x40001057));

	int timeout = 100000;
	do
	{
		if( *v ) break;
		if( !timeout ) break;
		asm volatile( "" : "=r"(timeout) );
		if( *v ) break;
		timeout--;
	} while( !*v );
	switch_time = SysTick->CNT;

	// Make sure GCC don't do any jibber jabber.
	//asm volatile( "" : "=r"(switch_time) : : );

	funPinMode( PA7, GPIO_ModeOut_PP_5mA );

	return switch_time - start;
}
#endif

