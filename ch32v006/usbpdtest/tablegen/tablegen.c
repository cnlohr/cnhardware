#include <stdio.h>
#include <stdint.h>

int main()
{
/*
	struct decodeStateBits
	{
		uint8_t run;      // 0..7 (length of time for this transition)
		uint8_t last;     // Last transition value.
		uint8_t expectShort;

		// This could be encoded as a 32x8bit table.
	};
*/
	FILE * dc = fopen( "../decodetable.h", "w" );
	fprintf( dc, "#ifndef _DECODETABLE_H\n" );
	fprintf( dc, "#define _DECODETABLE_H\n" );
	fprintf( dc, "\n" );

	int instate = 0;
	for( instate = 0; instate < 64; instate++ )
	{
		if( ( instate & 0xf ) == 0 ) fprintf( dc, "\n\t" );

		// lsb IN is current bit, lsb OUT is EMIT
		int val =  ( instate >> 0 ) & 1;
		int last = ( instate >> 1 ) & 1;
		int expectShort = ( ( instate >> 2 ) & 1 );
		int run = ( instate >> 3 ) & 7;

		int emit = 0;
		int bit = 0;
		int fault = 0;

		if( val != last )
		{
			if( expectShort )
			{
				if( run < 2 )
				{
					emit = 1;
					bit = 1;
				}
				else
				{
					emit = 1;
					fault = 1;
				}
				expectShort = 0;
			}
			else
			{
				if( run < 2 )
				{
					expectShort = 1;
				}
				else if( run < 4 )
				{
					emit = 1;
					bit = 0;
				}
				else
				{
					fault = 1;
				}
			}
			run = 0;
		}
		else
		{
			run++;
			if( run > 7 ) run = 7;
		}
		last = val;
		uint8_t outcode = emit | (last<<1) | (expectShort<<2) | (run<<3) | (bit<<6) | (fault<<7);
		printf( "%d %d (%08x)\n", val, last, outcode );
		fprintf( dc, "0x%02x, ", outcode );
	}
	fprintf( dc, "\n};\n" );


	fprintf( dc, "#endif\n" );
}


