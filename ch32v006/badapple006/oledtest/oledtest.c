/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

#define SSD1306_CUSTOM_INIT_ARRAY 1
#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_FULLUSE
#define SSD1306_W (64) 
#define SSD1306_H (64)
#define SSD1306_OFFSET 32

#define SSD1306_RST_PIN PC0

#include "ssd1306_i2c_bitbang.h"
#include "ssd1306.h"

#define RANDOM_STRENGTH 2
#include "lib_rand.h"

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
		0x20, 0x00, // Horizontal addresing mode
		0x00, 0x12, 0x40, 0xB0,
		0xD5, 0xf0, // Function Selection   <<< This controls scan speed. F0 is fastest.
		0xA8, 0x2F, // Set Multiplex Ratio
		0xD3, 0x00, // Set Display Offset
		0x40,
		0xA1, // Segment remap
		0xC8, // Set COM output scan direction
		0xDA, 0x12, // Set COM pins hardware configuration
		0x81, 0xCF, // Contrast control
		0xD9, 0x22, // Set Pre-Charge Period
		0xDB, 0x30, // Set VCOMH Deselect Level
		0xA4, // Entire display on (a5)/off(a4)
		0xA6, // Normal (a6)/inverse (a7)
		0x8D, 0x14, // Set Charge Pump
		0xAF, // Display On
	};

#if 0
	int i;
	for( i = 0; i < sizeof( ssd1306_init_array ); i++ )
	{
		int r = ssd1306_cmd( ssd1306_init_array[i] );
		if( r )
		{
			printf( "Failed to send OLED command at %d\n", i );
		}
	}
#else
	// Trying another mode
	if( ssd1306_pkt_send( ssd1306_init_array, sizeof(ssd1306_init_array), 1 ) )
	{
		printf( "Failed to setup\n" );
	}

	ssd1306_cmd(SSD1306_PAGEADDR);
	ssd1306_cmd(0); // Page start address (0 = reset)
	ssd1306_cmd(7); // Page end address

#endif
	int x, y;
	int t = 0;


	int32_t nextFrame = SysTick->CNT;
	uint32_t freeTime;

	#define DROPCOUNT 16
	static int drops[DROPCOUNT][3]; // x,y,age
	static int drophead = 0;

int ict = 0;

	while(1)
	{

		#if 0
		for( y = 0; y < 64; y++ )
		{
			for( x = 0; x < 64; x+=8 )
			{
				ssd1306_buffer[(x>>3)+y*64/8] =
					 //(((t)+(x>>3))&1)?0xaa:0x55;
					//((t & 0x3f) == y)?0xff:0;
					1<<(t&7);
			}
		}
		#endif

		//memset( ssd1306_buffer, 0, sizeof( ssd1306_buffer ) );

		uint32_t sz = sizeof( ssd1306_buffer )/4;
		uint32_t i;
		for( i = 0; i < sz; i++ )
		{
			((uint32_t*)ssd1306_buffer)[i] = 0;
		}

		char st[12];
		sprintf( st, "%d", freeTime );
		ssd1306_drawstr( 0, 0, st, 1);
if( ict < 10 )
{
		if( (t & 0x1f) == (rand()%0x1f) )
		{
			drophead = (drophead+1)&(DROPCOUNT-1);
			int * dr = drops[drophead];
			dr[0] = rand()%64;
			dr[1] = rand()%48;
			dr[2] = 1;
			ict++;
		}

		int d;
		for( d = 0; d < DROPCOUNT; d++ )
		{
			int * dr = drops[d];
			if( !dr[2] ) continue;
			dr[2]++;
			if( dr[2] > 80 ) dr[2] = 0;
		}
}

		int d;
		for( d = 0; d < DROPCOUNT; d++ )
		{
			int * dr = drops[d];
			if( !dr[2] ) continue;
			ssd1306_drawCircle( dr[0], dr[1], dr[2], 1 );
		}

		//memset( ssd1306_buffer, ((t+x+y)&1)?0xaa:0x55, sizeof( ssd1306_buffer ) );
		t++;
		//ssd1306_refresh();
		ssd1306_cmd(SSD1306_COLUMNADDR);
		ssd1306_cmd(SSD1306_OFFSET);   // Column start address (0 = reset)
		ssd1306_cmd(SSD1306_OFFSET+SSD1306_W-1); // Column end address (127 = reset)

		freeTime = nextFrame - SysTick->CNT;
		while( (int32_t)(SysTick->CNT - nextFrame) < 0 );
		nextFrame += 400000; 
		// 1600000 is 30Hz
		// 800000 is 60Hz
		// 533333 is 90Hz -- 90Hz seems about tha max you can go.
		// 400000 is 120Hz

ssd1306_pkt_send( (const uint8_t[]){0xD3, 0x32}, 2, 1 ); // Move the cursor "off screen"
ssd1306_pkt_send( (const uint8_t[]){0xA8, 0x01}, 2, 1 ); // Scan over two scanlines to hide updates

// Send data
ssd1306_data(ssd1306_buffer, 64*48/8);

ssd1306_pkt_send( (const uint8_t[]){0xD3, 0x3e}, 2, 1 ); // Make it so it "would be" off screen but only by 2 pixels.
ssd1306_pkt_send( (const uint8_t[]){0xA8, 0x31}, 2, 1 ); // Overscan screen by 2 pixels, but release from 2-scanline mode.
	}
}

