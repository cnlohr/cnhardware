#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../../tools/font_maker/src-lib/stb_image.h"

int w, h, channels_in_file;
uint32_t * image;

int boolpix( int x, int y )
{
	if( ( image[x+y*w] & 0xff00 ) > 128 ) return 1;
	return 0;
}


int goffsets[256];
int gsizes[256];
int goffsets_outs[256];

uint32_t gdata[2048];

int main( int argc, char ** argv )
{
	if( argc != 2 )
	{
		fprintf( stderr, "Error: Usage: [in, .png]\n" );
		return -5;
	}
	
	image = (uint32_t*)stbi_load( argv[1], &w, &h, &channels_in_file, 4 );

	if( !image )
	{
		fprintf( stderr, "Error: Couldn't find file %s\n", argv[1] );
		return -1;
	}

	int useglyphs[] = {
		' ', '.', ',', '!',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '/', '-',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	};

	int input_glyph_offset = 32;
	int outlen = 0;
	int i;
	int maxg = 0;
	for( i = 0; i < sizeof(useglyphs)/sizeof(useglyphs[0]); i++ )
	{
		int g = useglyphs[i];

		if( g > maxg ) maxg = g;

		// Scan across bottom bar of fonts
		int cur_glyph = 0;
		int lb = 0;
		int start = 0;
		int length = 0;
		int hitting = 0;
		int j;
		for( j = 0; j < w; j++ )
		{
			int bp = boolpix( j, h-1 );
			if( lb && !bp )
			{
				cur_glyph++;
			}
			else if( !lb && bp )
			{
				// White space (do nothnig)
			}
			lb = bp;
			if( cur_glyph + input_glyph_offset == g && bp == 0 )
			{
				if( !hitting ) start = j;
				else length++;
				hitting = 1;
			}
		}

		goffsets[g] = start;
		gsizes[g] = (length)?(++length):0;
		goffsets_outs[g] = outlen;

		int k;
		int x = start;
		for( j = 0; j < length; j++ )
		{
			for( k = 0; k < 24; k++ )
			{
				gdata[outlen] |= (!boolpix( x, k )) << k;
			}
			outlen++, x++;
		}

		fprintf( stderr, "Char %c start: %d len: %d\n", g, start, length );
	}

	if( maxg * 2 >= outlen )
	{
		fprintf( stderr, "Error: can't store asciimap in table this big.  Need more glyphs or a smaller max glyph.\n" );
		return -5;
	}
	// Glyph offset[10] + width[6] is encoded in gdata's MSBs

	int g;
	for( g = 0; g < maxg; g++ )
	{
		int start = goffsets_outs[g];
		int len = gsizes[g];
		uint16_t gword = start + (len<<10);
		if( start >= 1023 )
		{
			fprintf( stderr, "Error: Glyph map too large\n" );
			return -7;
		}
		if( len > 31 )
		{
			fprintf( stderr, "Error: Glyph too large\n" );
			return -7;
		}
		gdata[g*2+0] |= (gword & 0xff) << 24;
		gdata[g*2+1] |= (gword >> 8) << 24;
	}


	printf( "#include <stdint.h>\n" );
	printf( "const uint32_t swadge2025font[] = {\n\t" );
	for( i = 0; i < outlen; i++ )
	{
		int last = i == outlen-1;
		int lineend = (i & 7) == 7;
		printf( "0x%08x,%s%s", gdata[i], lineend ? "\n" : " ", (lineend && !last) ? "\t" : "" );
	}
	printf( "};\n" );

	fprintf( stderr, "Font uses %d lines (%d bytes)\n", outlen, outlen * 4 );

	return 0;
}

