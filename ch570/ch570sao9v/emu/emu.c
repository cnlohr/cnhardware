#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "os_generic.h"
#include "emu_ch570.h"

uint8_t ch570flash[FLASH_SIZE];
uint8_t ch570ram[RAM_SIZE];
struct MiniRV32IMAState ch570state;
og_thread_t ch570thread;


uint32_t R32_PA_PU;
uint32_t R32_PA_PD_DRV;
uint32_t R32_PA_DIR;
uint32_t R32_PA_OUT;
uint32_t R32_PFIC_IENR1;

volatile int ch32v003quitMode = 0;
volatile int ch570runMode;
volatile uint32_t pressures[4];

#define OLED_W 128
#define OLED_H 128
uint8_t oled[OLED_W*OLED_H/8];

#define CNFG_IMPLEMENTATION

#include "rawdraw_sf.h"

void HandleKey( int keycode, int bDown )
{
	const int pDown = 10000;
	switch( keycode )
	{
		case 'z': case 'Z': pressures[0] = bDown ? pDown : 0; break;
		case 'x': case 'X': pressures[1] = bDown ? pDown : 0; break;
		case 'c': case 'C': pressures[2] = bDown ? pDown : 0; break;
	}
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

int HandleDestroy()
{
	return 0;
}

void Handle_R8_TMR_CTRL_MOD( uint8_t regset )
{
//	R8_TMR_CTRL_MOD = 0b00000010; // Reset Timer
//	R8_TMR_CTRL_MOD = 0b11000101; // Capture mode rising edge

#if 0
	if( regset & 1 )
	{
		uint64_t tm = (ch570state.timerl) | (((uint64_t)ch570state.timerh)<<32);

		tm += 600;

		ch570state.mie |= (1 << 7);
		ch570state.mstatus |= 0x8;

		printf( "Setup Match\n" );
		ch570state.timermatchl = tm & 0xffffffff;
		ch570state.timermatchh = tm >> 32;
	}
#endif
}

void Handle_R8_SPI_BUFFER( uint8_t regset )
{
	int is_data = 0;
	if( R32_PA_OUT & 0x400 )
		is_data = 1;

	static int oled_ptr = 0;

	if( !is_data )
	{
		if( ( regset & 0xf0 ) == 0xb0 )
		{
			oled_ptr = (regset & 0xf)*128;
		}
	}
	else
	{
		if( oled_ptr < sizeof( oled ) )
		{
			oled[oled_ptr] = regset;
		}
		oled_ptr++;
	}
}

void * core_execution( void * )
{

    double dLast = OGGetAbsoluteTime();
    while (ch32v003quitMode == 0)
    {
        double dNow  = OGGetAbsoluteTime();
        uint32_t tus = (dNow - dLast) * 1000000;

		if( ch570runMode )
		{
	        int r = MiniRV32IMAStep(&ch570state, 0, 0, tus, 24 * tus);
    	  //  printzf( "STEP: %d %d\n", r, tus );
		}
        OGUSleep(100);

        dLast = dNow;
    }
}

int FormatDraw( int x, int y, int s, const char * format, ... )
{
	CNFGPenX = x;
	CNFGPenY = y;
	char sprintfbuffer[8192];
	va_list ap;
	va_start( ap, format );
	int r = vsnprintf(sprintfbuffer, sizeof( sprintfbuffer )-1, format, ap );
	va_end( ap );
	CNFGDrawText( sprintfbuffer, s );
	return r;
}

int main( int argc, char ** argv )
{
	if( argc < 2 )
	{
		fprintf( stderr, "Error: Usage: [emu] [binary image]\n" );
		return -1;
	}

	char * firmwareFile = argv[1];
	FILE * f = fopen( "../firmware/firmware.bin", "rb" );
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	uint8_t * firmware = malloc( len );
	if( fread( firmware, 1, len, f ) != len )
	{
		fprintf( stderr, "Error loading firmware \"%s\"", firmwareFile );
		exit( -5 );
	}
	fclose( f );
	ch570WriteMemory( firmware, len, 0x00000000 );
	ch570Resume();	

	ch570thread = OGCreateThread( core_execution, 0 );

	CNFGSetup( "ch570 sao emulator", 800, 512 );

	CNFGBGColor = 0x101010ff;

	while(CNFGHandleInput())
	{
		short w, h;
		CNFGGetDimensions( &w, &h );
		CNFGClearFrame();

		CNFGColor( 0xffffffff );
		{
			// Debug
			FormatDraw( 2, 2+16*0, 3, "RUN  %d\n", ch570runMode );
			FormatDraw( 2, 2+16*1, 3, "MST  %08x\n", ch570state.mstatus );
			FormatDraw( 2, 2+16*2, 3, "MEPC %08x\n", ch570state.mepc );
			FormatDraw( 2, 2+16*3, 3, "MTV  %08x\n", ch570state.mtval );
			FormatDraw( 2, 2+16*4, 3, "MCAU %08x\n", ch570state.mcause );
			FormatDraw( 2, 2+16*6, 3, "CYCL %08x\n", ch570state.cyclel );
			FormatDraw( 2, 2+16*7, 3, "CYCH %08x\n", ch570state.cycleh );
			FormatDraw( 2, 2+16*8, 3, "PC   %08x\n", ch570state.pc );
			FormatDraw( 2, 2+16*11, 3, "x0   %08x\n", ch570state.regs[0] );
			FormatDraw( 2, 2+16*12, 3, "lr   %08x\n", ch570state.regs[1] );
			FormatDraw( 2, 2+16*13, 3, "sp   %08x\n", ch570state.regs[2] );
			FormatDraw( 2, 2+16*14, 3, "gp   %08x\n", ch570state.regs[3] );
			FormatDraw( 2, 2+16*15, 3, "tp   %08x\n", ch570state.regs[4] );
			FormatDraw( 2, 2+16*16, 3, "t0   %08x\n", ch570state.regs[5] );
			FormatDraw( 2, 2+16*17, 3, "t1   %08x\n", ch570state.regs[6] );
			FormatDraw( 2, 2+16*18, 3, "t2   %08x\n", ch570state.regs[7] );
			FormatDraw( 2, 2+16*19, 3, "s0   %08x\n", ch570state.regs[8] );
			FormatDraw( 2, 2+16*20, 3, "s1   %08x\n", ch570state.regs[9] );
			FormatDraw( 2, 2+16*21, 3, "a0   %08x\n", ch570state.regs[10] );
			FormatDraw( 2, 2+16*22, 3, "a1   %08x\n", ch570state.regs[11] );
			FormatDraw( 2, 2+16*23, 3, "a2   %08x\n", ch570state.regs[12] );
			FormatDraw( 2, 2+16*24, 3, "a3   %08x\n", ch570state.regs[13] );
			FormatDraw( 2, 2+16*25, 3, "a4   %08x\n", ch570state.regs[14] );
			FormatDraw( 2, 2+16*26, 3, "a5   %08x\n", ch570state.regs[15] );
		}

		{
			// OLED
			int drx = 120;
			CNFGTackSegment( drx, 0, drx, h );
			int drw = w - drx;
			int drh = h;
			float aspectx = drw/((float)OLED_W);
			float aspecty = drh/((float)OLED_H);
			float aspect = aspecty;
			if( aspectx < aspect ) aspect = aspectx;
			int iaspect = (int)aspect;
			int cx = drx + drw/2;
			int cy = 0 + drh/2;
			int cornerx = cx - iaspect * OLED_W / 2;
			int cornery = cy - iaspect * OLED_H / 2;

			uint32_t oledimage[iaspect*iaspect*OLED_W*OLED_H];
			int x, y;
			for( y = 0; y < iaspect*OLED_H; y++ )
			for( x = 0; x < iaspect*OLED_W; x++ )
			{
				uint32_t col = 0xff000000;
				int ix = x / iaspect;
				int iy = y / iaspect;
				int pix = ((ix+(iy>>3)*OLED_W));
				int v = oled[pix] & (1<<(iy&7));
				if( v )
					col |= 0xffffffff;
				oledimage[x+y*iaspect*OLED_W] = col;
			}
			CNFGBlitImage( oledimage, cornerx, cornery, iaspect*OLED_H, iaspect*OLED_W );
		}

		CNFGSwapBuffers();
	}

	ch32v003quitMode = 1;

	OGJoinThread( ch570thread );

    return 0;
}
