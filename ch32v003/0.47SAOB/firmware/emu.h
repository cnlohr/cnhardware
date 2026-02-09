#ifndef _EMU_H
#define _EMU_H

#define CNFG_IMPLEMENTATION

#include <stdint.h>

#ifndef WASM
#include <sys/time.h>
#define mini_vsnprintf vsnprintf
uint32_t GetTimeUS()
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return (tv.tv_usec) + 1000000*(tv.tv_sec); 
}
#else
#include <stdint.h>
double OGGetAbsoluteTime();
uint32_t GetTimeUS()
{
	return (uint32_t)OGGetAbsoluteTime();
}
void exit( int i ) { while(1); }
#include <stdio.h>
#endif

#define rand sys_rand
#include "ch32v003fun/examples_v30x/turbo_adc_with_usb/testtop/rawdraw_sf.h"
#undef rand

void ssd1306_pkt_send( unsigned char * a, int b, int c ) { }
void ssd1306_rst() { }

uint32_t CLOCK_TICKS;
uint32_t GPIOA_INDR = 6;
short w, h;

struct 
{
	uint32_t CNT;
} * SysTick = (void*)&CLOCK_TICKS;

struct
{
	uint32_t INDR;
} * GPIOA = (void*)&GPIOA_INDR;


uint32_t highscore = 4269;
int32_t * const highScorePtr = (int32_t*)&highscore;

#include "graphics.h"

#ifdef WASM
void HandleKey( int keycode, int bDown ) __attribute__((export_name("HandleKey")));
void HandleButton( int x, int y, int button, int bDown ) __attribute__((export_name("HandleButton")));
int32_t getHighScore( );
void saveHighScore( int hs );
void WriteHighScore( int32_t hs ) { highscore = hs; saveHighScore( hs ); }
#else
void WriteHighScore( int32_t hs ) { }
int32_t getHighScore( ) { return 0; }
#endif


void HandleKey( int keycode, int bDown )
{
	switch( keycode )
	{
	case 'j': case 'J': case 65361: case 37:
		if(!bDown ) GPIOA_INDR |= 2;
		if( bDown ) GPIOA_INDR &=~2;
		break;
	case 'k': case 'K': case 65363: case 39:
		if(!bDown ) GPIOA_INDR |= 4;
		if( bDown ) GPIOA_INDR &=~4;
		break;
	}
}

void HandleButton( int x, int y, int button, int bDown )
{
	if( w/2 > x )
	{
		if(!bDown ) GPIOA_INDR |= 2;
		if( bDown ) GPIOA_INDR &=~2;
	}
	else
	{
		if(!bDown ) GPIOA_INDR |= 4;
		if( bDown ) GPIOA_INDR &=~4;
	}		
}

void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

#define SCALEUP 8
void SystemInit(void)
{
	CNFGSetup( "Example App", 72*SCALEUP, 40*SCALEUP );
	CLOCK_TICKS = GetTimeUS()*6;
	highscore = getHighScore();

}

// When running in webpage, can't be on stack
uint32_t framedata[40*SCALEUP][72*SCALEUP];

void EmuUpdate(void)
{
	if( !CNFGHandleInput() ) exit( 0 );
	CNFGBGColor = 0x000000ff; //Dark Blue Background
	CNFGClearFrame();
	CNFGGetDimensions( &w, &h );
	CNFGColor( 0xffffffff ); 

	int x, y;
	for( y = 0; y < 40; y++ )
	{
		for( x = 0; x < 72; x++ )
		{
			int p = (ssd1306_buffer[x+(y>>3)*72] >> (y & 7))&1;
			int ax, ay;
			for( ay = 0; ay < SCALEUP; ay++ )
			for( ax = 0; ax < SCALEUP; ax++ )
			{
#ifdef WASM
				framedata[(y*SCALEUP+ay)][x*SCALEUP+ax] = p ? 0xffffffff : 0x000000ff;
#else
				framedata[(y*SCALEUP+ay)][x*SCALEUP+ax] = p ? 0xffffffff : 0x00000000;
#endif
			}
		}
	}

	CNFGBlitImage( framedata[0], 0, 0, 72*SCALEUP, 40*SCALEUP );

	CNFGSwapBuffers();


	uint32_t now = GetTimeUS();
	CLOCK_TICKS = now*6+(now%6);
}

#define funGpioInitAll()
#define PD0 0
#define GPIO_CFGLR_OUT_50Mhz_PP 3
#define FUN_LOW 0
#define FUN_HIGH 1
#define funDigitalWrite( x, y )
#define funPinMode( a, b )
#define ssd1306_spi_init() 0

#endif

