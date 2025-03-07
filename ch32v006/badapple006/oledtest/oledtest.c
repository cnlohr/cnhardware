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

#define SSD1306_RST_PIN PC3

#define I2CDELAY_FUNC( x )

#include "ssd1306mini.h"
#include "ssd1306.h"


#define RAINDROPS 1
// If not defined, does a grey test.

#define RANDOM_STRENGTH 2
#include "lib_rand.h"

int main()
{
	SystemInit();
	funGpioInitAll();

	ssd1306_mini_i2c_setup();

	static const uint8_t ssd1306_init_array[] =
	{
		0xAE, // Display off
		0x20, 0x00, // Horizontal addresing mode
		0x00, 0x12, 0x40, 0xB0,
		0xD5, 0xf0, // Function Selection   <<< This controls scan speed. F0 is fastest.  The LSN = D divisor.
		0xA8, 0x2F, // Set Multiplex Ratio
		0xD3, 0x00, // Set Display Offset
		0x40,
		0xA1, // Segment remap
		0xC8, // Set COM output scan direction
		0xDA, 0x12, // Set COM pins hardware configuration
		0x81, 0xcf, // Contrast control
		//0xD9, 0x22, // Set Pre-Charge Period  (Not used)
		0xDB, 0x30, // Set VCOMH Deselect Level
		0xA4, // Entire display on (a5)/off(a4)
		0xA6, // Normal (a6)/inverse (a7)
		0x8D, 0x14, // Set Charge Pump
		0xAF, // Display On
		SSD1306_PAGEADDR, 0, 7, // Page setup, start at 0 and end at 7
	};

	// Trying another mode
	ssd1306_mini_pkt_send( ssd1306_init_array, sizeof(ssd1306_init_array), 1 );

	//int x, y;
	int t = 0;


	int32_t nextFrame = SysTick->CNT;
	uint32_t freeTime;

	#define DROPCOUNT 16
	static int drops[DROPCOUNT][3]; // x,y,age
	static int drophead = 0;

	int totalcircles = 0;

	while(1)
	{
		#define profile 0

		uint32_t sz = sizeof( ssd1306_buffer )/4;
		uint32_t i;

#if RAINDROPS

		for( i = 0; i < sz; i++ )
		{
			((uint32_t*)ssd1306_buffer)[i] = 0;
		}

		if( !profile || totalcircles < 8 )
		{
			if( (t & 0xf) == (rand()%0xf) )
			{
				drophead = (drophead+1)&(DROPCOUNT-1);
				int * dr = drops[drophead];
				dr[0] = rand()%64;
				dr[1] = rand()%48;
				dr[2] = 1;
				totalcircles++;
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
		if( profile )
		{
			char st[12];
			mini_itoa( freeTime, 10, 0, 1, st );
			ssd1306_drawstr( 0, 0, st, 1);
		}
		//uint32_t profile_start = SysTick->CNT;
		for( int d = 0; d < DROPCOUNT; d++ )
		{
			int * dr = drops[d];
			if( !dr[2] ) continue;
			ssd1306_drawCircle( dr[0], dr[1], dr[2], 1 );

			//ssd1306_fillRect( dr[0]-dr[2], dr[1]-dr[2], dr[2]*2, dr[2]*2, 1 );
			//ssd1306_drawLine( dr[0], dr[1], dr[0]-dr[2], dr[1]-dr[2]/2, 1 );
		}


		//freeTime = SysTick->CNT - profile_start;

		//memset( ssd1306_buffer, ((t+x+y)&1)?0xaa:0x55, sizeof( ssd1306_buffer ) );
#else
#endif

		t++;

		freeTime = nextFrame - SysTick->CNT;
		while( (int32_t)(SysTick->CNT - nextFrame) < 0 );
		nextFrame += 400000; 
		// 1600000 is 30Hz
		// 800000 is 60Hz
		// 533333 is 90Hz -- 90Hz seems about the max you can go with default 0xd5 settings.
		// 400000 is 120Hz -- Only possible when cranking D5 at 0xF0 and 0xa8 == 0x31
		// 320000 is 150Hz -- A biiig stretch. but on some displays achievable.
		// 266666 is 180Hz -- DOES NOT WORK

		// Move the cursor "off screen"
		// Scan over two scanlines to hide updates

		ssd1306_mini_pkt_send( 
			(const uint8_t[]){0xD3, 0x32, 0xA8, 0x01,
			// Column start address (0 = reset)
			// Column end address (127 = reset)
			SSD1306_COLUMNADDR, SSD1306_OFFSET, SSD1306_OFFSET+SSD1306_W-1, 0xb0 },
			8, 1 );


		if( t == 2 ) t = 0;
#if RAINDROPS
		// May be needed at lower framerates.
		Delay_Us(200);

		// Send data
		ssd1306_mini_data(ssd1306_buffer, 64*48/8);
#else
		ssd1306_mini_i2c_sendstart();
		ssd1306_mini_i2c_sendbyte( SSD1306_I2C_ADDR<<1 );
		ssd1306_mini_i2c_sendbyte( 0x40 ); // Data

		Delay_Us(100);
		int n;
		uint8_t go = (t == 0 || t == 2) ? 0xff : 0x00;
		for( n = 0; n < 6*8*8; n++ )
			ssd1306_mini_i2c_sendbyte( go );

		ssd1306_mini_i2c_sendstop();
#endif

		// Make it so it "would be" off screen but only by 2 pixels.
		// Overscan screen by 2 pixels, but release from 2-scanline mode.
		ssd1306_mini_pkt_send( (const uint8_t[]){0xD3, 0x3e, 0xA8, 0x31}, 4, 1 );  // There are other conditions where this is better if it's 0xA8 0x31.
	}
}

