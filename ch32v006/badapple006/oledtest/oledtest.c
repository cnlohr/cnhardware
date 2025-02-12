/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

#define SSD1306_CUSTOM_INIT_ARRAY 1
#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_FULLUSE
#define SSD1306_W (64) 
#define SSD1306_H (48)
#define SSD1306_OFFSET 0

#define SSD1306_RST_PIN PC0

#include "ssd1306_i2c_bitbang.h"
#include "ssd1306.h"

int main()
{
	SystemInit();
	funGpioInitAll();

	// On the 006, only GPIO_CFGLR_OUT_10Mhz_PP is available.

//	funPinMode( PC3, GPIO_CFGLR_OUT_10Mhz_PP ); // VDD On
//	funDigitalWrite( PC3, 1 );
	//funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP ); // VDD On
	//funDigitalWrite( PC0, 1 );
	Delay_Ms(100);
	ssd1306_rst();
	ssd1306_i2c_setup();
	Delay_Ms(100);
	if( ssd1306_init() )
	{
		printf( "OLED FAILURE\n" );
	}
	else
	{
		printf( "OLED OK\n" );
	}

	static const uint8_t ssd1306_init_array[] =
	{
		0xAE, // Display off
		//0x00, 0x12, 0x40, 0xB0,
		0x81, 0xCF, // Contrast control
		0xA1, // Segment remap
		0xA4, // Entire display on (a5)/off(a4)
		0xA6, // Normal (a6)/inverse (a7)
		0xA8, 0x2F, // Set Multiplex Ratio
		0xC8, // Set COM output scan direction
		0xD3, 0x00, // Set Display Offset
		0xD5, 0x80, // Function Selection
		0xD9, 0x22, // Set Pre-Charge Period
		0xDA, 0x12, // Set COM pins hardware configuration
		0xDB, 0x30, // Set VCOMH Deselect Level
		0x8D, 0x14, // Set Charge Pump
		0xAF, // Display On
	};

	int i;
	for( i = 0; i < sizeof( ssd1306_init_array ); i++ )
	{
		int r = ssd1306_cmd( ssd1306_init_array[i] );
		if( r )
		{
			printf( "Failed to send OLED command at %d\n", i );
		}
	}

	int x, y;
	int t = 0;

	while(1)
	{
		for( y = 0; y < 64; y++ )
		{
			for( x = 0; x < 96; x+=8 )
			{
				ssd1306_buffer[(x>>3)+y*8] = 0xaa;//(y&1)?0xfa:0x00;
			}
		}
		memset( ssd1306_buffer, 0xAA, sizeof(ssd1306_buffer) );
		t++;
		ssd1306_refresh();
	}
}

