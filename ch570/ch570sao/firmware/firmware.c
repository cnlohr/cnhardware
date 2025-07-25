/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

#define SSD1306_I2C_ADDR 0x3D

#define SSD1306_W 72
#define SSD1306_H 40
#define SSD1306_FULLUSE
#define SSD1306_OFFSET 28

// control pins

#define SSD1306_I2C_BITBANG_SDA PA2
#define SSD1306_I2C_BITBANG_SCL PA3
#define SSD1306_RST_PIN         PA4 // Not really, but just to make it happy.

#define I2CDELAY_FUNC(x) 	//ADD_N_NOPS(x)

// Force functions to RAM.
unsigned char ssd1306_i2c_sendbyte( unsigned char data ) __attribute__((section(".srodata")));
uint8_t ssd1306_pkt_send(const uint8_t *data, int sz, uint8_t cmd) __attribute__((section(".srodata")));

#include "ssd1306_i2c_bitbang.h"
#include "ssd1306.h"

uint32_t count;


#if 0
#define I2CNEEDSCAN
#define PIN_SCL PA3
#define PIN_SDA PA2
#define DELAY1 Delay_Us(1);
#define DELAY2 Delay_Us(2);
#define DSCL_IHIGH  { funPinMode( PIN_SCL, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SCL, 1 ); } 
#define DSDA_IHIGH  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSDA_INPUT  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSCL_OUTPUT { funDigitalWrite( PIN_SCL, 0 ); funPinMode( PIN_SCL, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define DSDA_OUTPUT { funDigitalWrite( PIN_SDA, 0 ); funPinMode( PIN_SDA, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define READ_DSDA    funDigitalRead( PIN_SDA )
#define I2CNEEDGETBYTE 1
#define I2CNEEDSCAN    1

#include "static_i2c.h"

// Scan();

#endif

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	funPinMode( PA7, GPIO_CFGLR_IN_PU );
	
	ssd1306_i2c_setup();
	ssd1306_init();

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


	Delay_Ms(2);

	int frameno = 0;

	int debug = 0;

	unsigned start;

	while(1)
	{
		//ssd1306_refresh();

		char st[128];
		sprintf( st, "%08x", (int)SysTick->CNT );
		ssd1306_drawstr_sz(0, 0, st, 1, 2 );

		sprintf( st, "%d", debug>>8 );
		ssd1306_drawstr_sz(0, 24, st, 1, 2 );

		start = SysTick->CNT;
//		ssd1306_refresh();
		ssd1306_cmd(SSD1306_COLUMNADDR);
		ssd1306_cmd(28);   // Column start address (0 = reset)
		ssd1306_cmd(28+71); // Column end address (127 = reset)
		ssd1306_cmd(SSD1306_PAGEADDR);
		ssd1306_cmd(0); // Page start address (0 = reset)
		ssd1306_cmd(5); // Page end address
		ssd1306_data(ssd1306_buffer, sizeof(ssd1306_buffer));

		debug = SysTick->CNT - start;
		frameno++;
	}
}
