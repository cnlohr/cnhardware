/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

#define SH1107_128x128

#define SSD1306_RST_PIN  PA6
#define SSD1306_CS_PIN   PA4
#define SSD1306_DC_PIN   PA10
#define SSD1306_MOSI_PIN PA7
#define SSD1306_SCK_PIN  PA5

//#define SSD1306_SOFT_SPI 1

#include "ssd1306_spi.h"
#include "ssd1306.h"

uint32_t count;





/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
// FakeDC
//
// Fake digital-to-analog, by looking at the time constant on a capcaitor
// and waiting for the scmitt trigger flip.
//

#define CAPACITANCE 0.00000001
#define RESISTANCE  33000.0
#define VREF 1.75
#define FIXEDPOINT_SCALE 100

volatile uint32_t lastfifo = 0;

void TMR1_IRQHandler(void) __attribute__((interrupt))  __attribute__((section(".srodata")));
void TMR1_IRQHandler(void)
{
	R8_TMR_INT_FLAG = 2;
	lastfifo = R32_TMR_FIFO;
	funPinMode( PA2, GPIO_ModeOut_PP_20mA );
}


// The timing on the setup has to be tight.
void EventRelease(void) __attribute__((section(".srodata"))) __attribute__((noinline));
void EventRelease(void)
{
	R8_TMR_CTRL_MOD = 0b00000010; // Reset Timer
	R8_TMR_CTRL_MOD = 0b11000101; // Capture mode rising edge
	funPinMode( PA2, GPIO_ModeIN_Floating );
}

void SetupADC(void)
{
	R8_TMR_CTRL_MOD = 0b00000010; // All clear
	R32_TMR_CNT_END = 0x03FFFFFF; // Maximum possible counter size.
	R8_TMR_CTRL_MOD = 0b11000101; // Capture mode rising edge
	R8_TMR_INTER_EN = 0b10; // Capture event.

	R16_PIN_ALTERNATE_H |= 1<<6; // Map PA2 to CAP1 (could be PA7, PA4, or PA9) (see RB_TMR_PIN)

	NVIC_EnableIRQ(TMR1_IRQn);
	__enable_irq();

	funPinMode( PA2, GPIO_ModeOut_PP_20mA );
}

/////////////////////////////////////////////////////////////////////////////////////////////////















int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	funPinMode( PA11, GPIO_CFGLR_OUT_10Mhz_PP );
//	funDigitalWrite( PA11, 1 ); // BS1 = 1 for I2C
	funDigitalWrite( PA11, 0 ); // BS1 = 0 for SPI

	funPinMode( PA6, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PA6, 1 ); // RES
	funPinMode( PA4, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PA4, 0 ); // SCS
	funPinMode( PA10, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PA10, 0 ); // D/C



	ssd1306_rst();
	ssd1306_spi_init();
	ssd1306_init();
	ssd1306_setbuf(0x00);

	// Turbo-time
	ssd1306_cmd(0xD5);
	ssd1306_cmd(0xe0);


	ssd1306_cmd(0xc8);
	ssd1306_cmd(0xa1);


	SetupADC();

	int frameno = 0;

	int debug = 0;

	unsigned start;

	while(1)
	{

		start = SysTick->CNT;

		uint32_t pressures[4];

		int btn = 0;
		for( btn = 0; btn < 4; btn++ )
		{
			// try GPIO_CFGLR_IN_PUPD, GPIO_ModeIN_Floating, GPIO_CFGLR_OUT_10Mhz_PP as well
			funPinMode( PA3, GPIO_ModeIN_Floating );
			funPinMode( PA8, GPIO_ModeIN_Floating );
			funPinMode( PA9, GPIO_ModeIN_Floating );
			switch( btn )
			{
			case 0:
				funPinMode( PA8, GPIO_CFGLR_OUT_10Mhz_PP );
				funDigitalWrite( PA3, 1 );
				break;
			case 1:
				funPinMode( PA9, GPIO_CFGLR_OUT_10Mhz_PP );
				funDigitalWrite( PA8, 1 );
				break;
			case 2:
				funPinMode( PA3, GPIO_CFGLR_OUT_10Mhz_PP );
				funDigitalWrite( PA9, 1 );
				break;
			}

			if( *((uint32_t*)0x4fff0000) == 0xaaaaaaaa )
			{
				pressures[btn] = *((uint32_t*)(0x4fff0004+4*btn));
			}
			else
			{
				lastfifo = 0;
				EventRelease();
				int to = 4000;
				while( !lastfifo && --to );

				#define COEFFICIENT (const uint32_t)(FUNCONF_SYSTEM_CORE_CLOCK*(RESISTANCE*CAPACITANCE)*VREF*FIXEDPOINT_SCALE+0.5)
				int r = lastfifo - 2; // 2 cycles back.
				int vtot = COEFFICIENT/r + ((const uint32_t)(VREF*FIXEDPOINT_SCALE));
				pressures[btn] = vtot - 70;
			}
		}
		debug = SysTick->CNT - start;
		frameno++;
		
		ssd1306_setbuf(0x00);

		char st[128];
		sprintf( st, "%08x", (int)SysTick->CNT );
		ssd1306_drawstr_sz(0, 0, st, 1, 2 );

		sprintf( st, "%3d %d", debug>>8, (int)pressures[3] );
		ssd1306_drawstr_sz(0, 24, st, 1, 2 );


		for( btn = 0; btn < 3; btn++ )
		{
			int x = 32 + btn * 32;
			int y = 90;
			int p = pressures[btn]>>9;
			//ssd1306_drawCircle( x, y, 
			ssd1306_fillCircle( x, y, p, 1 );
		}

		ssd1306_refresh();
	}
}
