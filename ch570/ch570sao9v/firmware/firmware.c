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
	ssd1306_setbuf(0xff);
	
//	Scan();
/*
	uint8_t *cmd_list = (uint8_t *)ssd1306_init_array;
	while(*cmd_list != SSD1306_TERMINATE_CMDS)
	{
		if(ssd1306_cmd(*cmd_list++))
		{
			printf( "CMD FAIL\n" );
		}
	}
	while(1);
*/
//	ssd1306_refresh();	

//	ssd1306_init();

#if 0
	ssd1306_cmd( SSD1306_SETMULTIPLEX );
	ssd1306_cmd( 39 );
	ssd1306_cmd( SSD1306_SETDISPLAYOFFSET );
	ssd1306_cmd( 0 );

	ssd1306_cmd( SSD1306_SETPRECHARGE ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETCONTRAST ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETVCOMDETECT ); ssd1306_cmd( 0x40 );
	ssd1306_cmd( 0xad ); ssd1306_cmd( 0x90 ); // Set Charge pump (set to 0x90 for extra bright)
	ssd1306_cmd( SSD1306_COMSCANINC );
	ssd1306_cmd( SSD1306_SEGREMAP );
#endif
	Delay_Ms(2);

	int frameno = 0;

	int debug = 0;

	unsigned start;

	while(1)
	{
		char st[128];
		sprintf( st, "%08x", (int)SysTick->CNT );
		ssd1306_drawstr_sz(0, 0, st, 1, 2 );

		sprintf( st, "%d", debug>>8 );
		ssd1306_drawstr_sz(0, 24, st, 1, 2 );
		start = SysTick->CNT;

		ssd1306_refresh();
#if 0
		ssd1306_cmd(SSD1306_COLUMNADDR);
		ssd1306_cmd(28);   // Column start address (0 = reset)
		ssd1306_cmd(28+71); // Column end address (127 = reset)
		ssd1306_cmd(SSD1306_PAGEADDR);
		ssd1306_cmd(0); // Page start address (0 = reset)
		ssd1306_cmd(5); // Page end address
#endif

		debug = SysTick->CNT - start;
		frameno++;
	}
}
