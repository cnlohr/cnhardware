/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

//#define SH1107
// Actually an SSD1327
#define SSD1327
#define SSD1306_FULLUSE
#define SSD1306_W (128) 
#define SSD1306_H (96*4)// For greyscale.
#define SSD1306_FULLUSE
#define SSD1306_OFFSET 0

#define SSD1306_RST_PIN PC3
#define SSD1306_CS_PIN PC1
#define SSD1306_DC_PIN PC2

#define SSD1306_MOSI_PIN PC6
#define SSD1306_SCK_PIN  PC5

#include "ssd1306_spi.h"
#include "ssd1306.h"

int main()
{
	SystemInit();
	funGpioInitAll();

	// On the 006, only GPIO_CFGLR_OUT_10Mhz_PP is available.

	funPinMode( PC4, GPIO_CFGLR_OUT_10Mhz_PP ); // BCTRL to 0.
	funDigitalWrite( PC4, 0 );

	// Power on
	funPinMode( PC0, GPIO_CFGLR_OUT_10Mhz_PP ); // Turn on OLED display.
	funDigitalWrite( PC0, 1 );

	ssd1306_rst();

	if( ssd1306_spi_init() || ssd1306_init() )
	{
		printf( "OLED FAILURE\n" );
	}

	static const uint8_t ssd1327_init_array[] =
	{

		0xAE, // Display off
		0xD5, 0x62, // Function Selection
		0xA8, 0x5F, // Set Multiplex Ratio
		0xA2, 0x20, // Set Display Offset
		0xA1, 0x60, // Set Display Start Line
		0xAB, 0x01, // Enable internal VDD regulator
		0xa0, 0x51, // Set Re-Map
		0xb1, 0x22, // Set phase length
		0x81, 0xdf, // Set Contrast Control
		0xbc, 0x10, // Set Pre-Charge voltage
		0xbe, 0x05, // Set VCOMH Deselect Level

		0xa4, // Normal display

		0xb6, 0x0a, // Set Second pre-charge Period
		0xaf, // Display On

//		0xa0, 0x51, // Set Re-Map

/*
		0xD5, 0x62, // Function Selection
		0xa0, 0x40, // Enable even-odd

		0xA2, 0x40, // Set Display Offset
		0xA1, 0x00, // Set Display Start Line
		0xAB, 0x01, // Enable internal VDD regulator

		0xb1, 0x22, // Set phase length
		0x81, 0xdf, // Set Contrast Control
		0xbc, 0x10, // Set Pre-Charge voltage
		0xbe, 0x05, // Set VCOMH Deselect Level

		// Bonus
		0xA8, 0x5F, // Set Multiplex Ratio
*/
	};
	int i;
	for( i = 0; i < sizeof( ssd1327_init_array ); i++ )
	{
		ssd1306_cmd( ssd1327_init_array[i] );
	}

	int x, y;
	int t = 0;

//	ssd1306_cmd( 0xA2 );
//	ssd1306_cmd( 0x60 );
//	ssd1306_cmd( 0xA1 );
//	ssd1306_cmd( 0x80 );


	while(1)
	{
		for( y = 0; y < 96; y++ )
		{
			for( x = 0; x < 128; x++ )
			{
				int gs = ((x*y+t)>>1)&0xf;
				if( x & 1 )
					ssd1306_buffer[(x>>1)+y*64] |= gs;
				else
					ssd1306_buffer[(x>>1)+y*64] = gs<<4;

				//ssd1306_buffer[(x>>1)+y*64] = y;//(y&1)?0xfa:0x00;
			}
		}
		t++;
		ssd1306_refresh();

/*	ssd1306_cmd( 0xA8 );
	ssd1306_cmd( 0x02 );
	ssd1306_cmd( 0xA8 );
	ssd1306_cmd( 0x3f );
	Delay_Us(100);
	ssd1306_cmd( 0xA8 );
	ssd1306_cmd( 0x02 );
*/
/*
	ssd1306_cmd( 0xA8 );
	ssd1306_cmd( 0x01 ); // Set Multiplex Ratio
	ssd1306_cmd( 0xA8 );
	ssd1306_cmd( 0x0f ); // Set Multiplex Ratio
	ssd1306_cmd( 0xA0 );
	ssd1306_cmd( 0x0f ); // Set start line
*/
	}
}

