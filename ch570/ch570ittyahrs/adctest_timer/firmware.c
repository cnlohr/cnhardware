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

#define CAPACITANCE 0.000000134
#define RESISTANCE  5100.0
#define VREF 0.1


// VERF = 100mV and PA7 - CMP_VERF
// Timer Capture comes from Input (not that it matters) + Enable.
//
// Rising edge generates interrupt. (Disable interrupt for now)
// After a lot of testing 100mV seems best.

#define CMP_CONFIG ( 0x2 << 4 ) | 0b1111 | (0b00<<10)

volatile uint32_t lastfifo = 0;

void TMR1_IRQHandler(void) __attribute__((interrupt))  __attribute__((section(".srodata")));
void TMR1_IRQHandler(void)
{
	R8_TMR_INT_FLAG = 2;
	lastfifo = R32_TMR_FIFO;
	funPinMode( PA7, GPIO_ModeOut_PP_20mA );
}


// The timing on the setup has to be tight.
void EventRelease(void) __attribute__((section(".srodata"))) __attribute__((noinline));
void EventRelease(void)
{
	R8_TMR_CTRL_MOD = 0b00000010; // Capture mode rising edge
	R8_TMR_CTRL_MOD = 0b11000101; // Capture mode rising edge
	funPinMode( PA7, GPIO_ModeIN_Floating );
}

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	R32_CMP_CTRL = CMP_CONFIG; // Disable ISR

	R8_TMR_CTRL_MOD = 0b00000010; // All clear
	R32_TMR_CNT_END = 0x03FFFFFF; // Maximum possible counter size.
	R8_TMR_CTRL_MOD = 0b11000101; // Capture mode rising edge
	R8_TMR_INTER_EN = 0b10; // Capture event.

	NVIC_EnableIRQ(TMR1_IRQn);
	__enable_irq();

	funPinMode( PA7, GPIO_ModeOut_PP_20mA );

	while(1)
	{
		Delay_Ms(1);
		EventRelease();
		Delay_Ms(1);

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
		#define COEFFICIENT (const uint32_t)(FUNCONF_SYSTEM_CORE_CLOCK*(RESISTANCE*CAPACITANCE)*VREF*1000)
		int r = lastfifo;
		int vtot = COEFFICIENT/r + 100; //100mV offset (vComp)
		printf( "%d %d\n", r, vtot );
	}
}


