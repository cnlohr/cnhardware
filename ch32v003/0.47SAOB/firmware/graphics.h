#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#define SSD1306_W 72
#define SSD1306_H 40
#define SSD1306_FULLUSE
#define SSD1306_OFFSET 28


// control pins
#define SSD1306_RST_PIN PC3
#define SSD1306_CS_PIN PC0
#define SSD1306_DC_PIN PC4
#define SSD1306_MOSI_PIN PC6
#define SSD1306_SCK_PIN  PC5
#define SSD1306_BAUD_RATE_PRESCALER SPI_BaudRatePrescaler_2

#ifndef EMU
#include "ssd1306_spi.h"
#endif

#include "ssd1306.h"
#include <stdarg.h>
#include <limits.h>
#include "font8x8_basic_trunrot.h"

#ifndef EMU
#include "ch32v003fun.h"
#endif

#include "swadge2025font.h"

#define RANDOM_STRENGTH 1

#include "lib_rand.h"

int frameno;
extern uint32_t gameTimeUs;
int glyphdraw_invert;
int glyphdraw_nomask;

// Organization:
//   B0, MSB  B1 B2 B3
//   B0
//   B0
//   B0
//   B0
//   B0
//   B0
//   B0, LSB
//   B72

extern uint8_t ssd1306_buffer[SSD1306_W*SSD1306_H/8];

int draw8Glyph( int x, int y, int c )
{
	const uint8_t * glyph = font8x8_basic_trunrot[c];
	int yo = y & 8;
	int yt = y / 8;

	int tx;

	if( yo )
	{
		if( yt > SSD1306_H/8 ) return 8;
		int oo = yt * 72 + x;
		for( tx = 0; tx < 8; tx++ )
		{
			int ox = x+tx;
			if( ox < 0 ) continue;
			if( ox > SSD1306_W ) break;
			int g = glyph[tx];
			g <<= 1;
			ssd1306_buffer[oo+tx] = g;
		}
	}

	yt++;
	if( yt > SSD1306_H/8 ) return 8;
	int oo = yt * 72 + x;

	for( tx = 0; tx < 8; tx++ )
	{
		int ox = x+tx;
		if( ox < 0 ) continue;
		if( ox > SSD1306_W ) break;
		int g = glyph[tx];
		g <<= 1;
		ssd1306_buffer[oo+tx] = g;
	}
	return 8;
}

#define SWADGEGLYPH_MARGIN 1
int swadgeGlyph( int x, int y, int c )
{
	int gindex = c*2;
	int ss = (swadge2025font[gindex+0]>>24) | ( (swadge2025font[gindex+1]>>24) << 8 );
	if( !ss ) return 0;
	int offset = ss & 0x3ff;
	int len = ss >> 10;
	if( y == INT_MAX ) return len + SWADGEGLYPH_MARGIN;
	const uint32_t * gptr = swadge2025font + offset;
	int end = x + len;
	if( end > SSD1306_W ) end = SSD1306_W;
	int startrow = (y+64)/8-8;
	uint8_t * bufferbase = &ssd1306_buffer[(x + startrow*SSD1306_W)];
	int mx;
	for( mx = x; mx < end + SWADGEGLYPH_MARGIN; mx++, gptr++, bufferbase++ )
	{
		if( mx < 0 ) continue;
		int yo = ((uint32_t)y) & 7;
		int row = 0;
		int maxrow = (yo ? 4 : 3) + startrow;
		if( maxrow > SSD1306_H/8 ) maxrow = SSD1306_H/8;

		uint32_t line = (*gptr &  0xffffff) << yo;

		if( mx >= end ) line = 0; // For margin draw.

		uint32_t emask = 0xff >> (8-yo);
		if( glyphdraw_nomask ) emask = 0xffffffff;
		uint32_t nextemask = 0xffff << (yo+16);
		uint8_t * buffer = bufferbase;

		for( row = startrow; row < maxrow; row++ )
		{
			if( row >= 0 )
			{
				uint32_t l = ((line) | ((buffer[0] & emask) ^ (glyphdraw_invert)))  ^ (glyphdraw_invert);
				*buffer = l;
			}
			line>>=8;
			buffer += SSD1306_W;
			emask>>=8;
			emask |= nextemask;
		}
	}
	return len + SWADGEGLYPH_MARGIN;
}


int swadgeGlyphHalf( int x, int y, int c )
{
	int gindex = c*2;
	int ss = (swadge2025font[gindex+0]>>24) | ( (swadge2025font[gindex+1]>>24) << 8 );
	if( !ss ) return 0;
	int offset = ss & 0x3ff;
	int len = ss >> (10+1); // len/2
	if( y == INT_MAX ) return len + SWADGEGLYPH_MARGIN;
	const uint32_t * gptr = swadge2025font + offset;
	int end = x + len;
	if( end > SSD1306_W ) end = SSD1306_W;
	int startrow = (y+64)/8-8;
	uint8_t * bufferbase = &ssd1306_buffer[(x + startrow*SSD1306_W)];
	int mx;
	for( mx = x; mx < end + SWADGEGLYPH_MARGIN; mx++, gptr+=2, bufferbase++ )
	{
		if( mx < 0 ) continue;
		int yo = ((uint32_t)y) & 7;
		int row = 0;
		int maxrow = 2 + startrow;
		if( maxrow > SSD1306_H/8 ) maxrow = SSD1306_H/8;

		uint32_t line = (*gptr &  0xffffff) << yo;

		if( mx >= end ) line = 0; // For margin draw.

		uint32_t emask = 0xff >> (8-yo);
		uint32_t nextemask = 0xffff << (yo+16);
		if( glyphdraw_nomask ) emask = 0xffffffff;
		uint8_t * buffer = bufferbase;

		for( row = startrow; row < maxrow; row++ )
		{
			if( row >= 0 )
			{
				// based on https://stackoverflow.com/a/45695465/2926815
				int l = line;
				l = ((l & 0x4444) >> 1) | ((l & 0x1111) >> 0);
				// x = 0b ..ab ..cd ..ef ..gh ..ij ..kl ..mn ..op

				l = ((l & 0x3030) >> 2) | ((l & 0x0303) >> 0);
				// x = 0b .... abcd .... efgh .... ijkl .... mnop

				l = ((l & 0x0F00) >> 4) | ((l & 0x000F) >> 0);
				// x = 0b .... .... abcd efgh .... .... ijkl mnop

				//l = ((l & 0x0000) >> 8) | ((l & 0x00FF) >> 0);
				// x = 0b .... .... .... .... abcd efgh ijkl mnop

				l = ((l) | ((buffer[0] & emask)^glyphdraw_invert))^glyphdraw_invert;
				buffer[0] = l;
			}
			line>>=16;
			buffer += SSD1306_W;
			emask>>=16;
			emask |= nextemask;
		}
	}
	return len + SWADGEGLYPH_MARGIN;
}

int swadgeDraw( int x, int y, int alignmode, int(*cfn)(int x, int y, int c), const char * format, ... )
{
	int tpl = 0;
	char buffer[32];
	va_list args;
	va_start( args, format );

#ifndef EMU
	int mini_vsnprintf();
#endif
	int tlen = mini_vsnprintf( buffer, sizeof( buffer ), format, args );
	va_end( args );

	int n;
	if( alignmode )
	{
		if( alignmode == 1 || alignmode == 2 )
		{
			int oy = y;
			int maxx = 0;
			for( n = 0; n < tlen; n++ )
			{
				int b = buffer[n];
				if( b == '\n' ) { y += 24; if( tpl > maxx ) maxx = tpl; tpl = 0; continue; }
				tpl += cfn( x + tpl, INT_MAX, b );
			}
			if( tpl > maxx ) maxx = tpl;
			if( alignmode == 1 )
				x -= maxx/2;
			else
				x -= maxx;
			y = oy;
		}
	}

	tpl = 0;
	for( n = 0; n < tlen; n++ )
	{
		int b = buffer[n];
		if( b == '\n' ) { y += 24; tpl = 0; continue; }
		tpl += cfn( x + tpl, y, b );
	}
	return tpl;
}






// Blit16 
// https://github.com/azmr/blit-fonts
/*
// TODO!
static uint16_t blit16font[95] = {
		0x0000,0x2092,0x002d,0x5f7d,0x279e,0x52a5,0x7ad6,0x0012,
		0x4494,0x1491,0x0aba,0x05d0,0x1400,0x01c0,0x0400,0x12a4,
		0x2b6a,0x749a,0x752a,0x38a3,0x4f4a,0x38cf,0x3bce,0x12a7,
		0x3aae,0x49ae,0x0410,0x1410,0x4454,0x0e38,0x1511,0x10e3,
		0x73ee,0x5f7a,0x3beb,0x624e,0x3b6b,0x73cf,0x13cf,0x6b4e,
		0x5bed,0x7497,0x2b27,0x5add,0x7249,0x5b7d,0x5b6b,0x3b6e,
		0x12eb,0x4f6b,0x5aeb,0x388e,0x2497,0x6b6d,0x256d,0x5f6d,
		0x5aad,0x24ad,0x72a7,0x6496,0x4889,0x3493,0x002a,0xf000,
		0x0011,0x6b98,0x3b79,0x7270,0x7b74,0x6750,0x95d6,0xb9ee,
		0x5b59,0x6410,0xb482,0x56e8,0x6492,0x5be8,0x5b58,0x3b70,
		0x976a,0xcd6a,0x1370,0x38f0,0x64ba,0x3b68,0x2568,0x5f68,
		0x54a8,0xb9ad,0x73b8,0x64d6,0x2492,0x3593,0x03e0, };

void drawBlit16( int x, int y, 
*/

void drawArc(int32_t x, int32_t y, int32_t radius_x65536, int32_t rstart, int32_t rstop, int8_t color)
{

	int a = 0;
	int b = radius_x65536;
	int i;
	if( rstop > 640 ) 
	{
		int rdif = (rstop / 320)-1;
		rstop -= rdif * 320;
		rstart -= rdif * 320;
	}
	// Similar to // https://amycoders.org/tutorials/sintables.html  (The last part of the last section)
	for( i = 0; i < rstop; i++ )
	{
		int lastb = b;

		// 1>>5 = 0.031250 -> 3.1415926*2.0 / 0.031250 = 201.06176
		// Instead I want to target 6*32 or 96*2 edges... well 95*2. -> (b>>5)+(b>>9)-(b>>13)
		// What about 10*32?
		a = a + ((b>>6)+(b>>8)+(b>>13));
		b = b - ((a>>6)+(a>>8)+(a>>13));
		if( i >= rstart && i < rstop )
		{
			int s = (a)>>16;
			int c = (b+lastb)>>17;
			ssd1306_drawPixel(x + s, y + c, color);
		}
	}

}

int backgroundAttrib;

void background( int mode )
{
	int i = 0, x, y;
	switch( mode )
	{
	case 1: // White
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = 0xff; 
		break;
	case 2: // Drops
		for(i=0;i<sizeof(ssd1306_buffer);i++)
			ssd1306_buffer[i] = (_rand_lfsr_update(), _rand_lfsr);
		break;
	case 3: // Noise
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = _rand_gen_nb(8); 
		break;
	case 4: // Checker
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = (i&1)?0xaa:0x55;
		break;
	case 5: // Grey
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = ((i+frameno)&1)?0xaa:0x55;
		break;
	case 6: // Racing
	case 7: // Racing left->right
	case 8: // Blaaa
	case 9: // Blaaa
	{
		uint8_t rdv = (const uint8_t[]){15, 31, 14, 16}[mode - 6];
		uint8_t rdw = (const uint8_t[]){14, 12, 15, 16}[mode - 6];
		uint8_t even = 0xff00>>(16-((gameTimeUs>>rdv)&15));
		uint8_t odd = ~even;
		for( y = 0; y < SSD1306_H/8; y++ )
		for( x = 0; x < SSD1306_W; x++ )
			ssd1306_buffer[i++] =
				((((x-(gameTimeUs>>rdw))>>3)+y)&1)
				?even:odd;
		break;
	}
	case 10: // Dark noise
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = 1<<(_rand_gen_nb(10));
		break;
	case 11: // Bright noise
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = ~(1<<(_rand_gen_nb(8)));
		break;
	case 12: // Rain down
	{
		_rand_lfsr = 0x5005;
		uint8_t col[SSD1306_W];		
		for(i=0;i<SSD1306_W;i++)
			col[i] = _rand_gen_nb( 8 );

		int i = 0;
		for( y = 0; y < SSD1306_H/8; y++ )
			for( x = 0; x < SSD1306_W; x++ )
				ssd1306_buffer[i++] = 0x80 >> ((col[x]+y*8-(gameTimeUs>>13)) & 0x3f);

		break;
	}
	case 13: // Dark scrolley, static
	{
		_rand_lfsr = 0x5005;
		int j = backgroundAttrib;
		for(i=0;i<sizeof(ssd1306_buffer) ;i++)
		{
			while( j >= sizeof(ssd1306_buffer) ) j-= sizeof(ssd1306_buffer);
			ssd1306_buffer[j] = 1<<(_rand_gen_nb(13));
			j++;
		}
		break;
	}
	case 0:
	default: // Black
		for(i=0;i<sizeof(ssd1306_buffer);i++) ssd1306_buffer[i] = 0;
		break;
	}
}


void swapBuffer()
{

	ssd1306_cmd(SSD1306_COLUMNADDR);
	ssd1306_cmd(28);   // Column start address (0 = reset)
	ssd1306_cmd(28+71); // Column end address (127 = reset)
	
	ssd1306_cmd(SSD1306_PAGEADDR);
	ssd1306_cmd(0); // Page start address (0 = reset)
	ssd1306_cmd(5); // Page end address

	// flip dir (Rotate 180)
	ssd1306_cmd( SSD1306_COMSCANINC );
	ssd1306_cmd( 0xA0 ); // Also flip
	ssd1306_cmd( 0x20 );
	ssd1306_cmd( 0x00 );

	for(int i=0;i<sizeof(ssd1306_buffer);i+=SSD1306_PSZ)
	{
		ssd1306_data(&ssd1306_buffer[i], SSD1306_PSZ);
	}
}

#endif
/*
blit16 Font

ISC License Copyright (c) 2018 Andrew Reece

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

