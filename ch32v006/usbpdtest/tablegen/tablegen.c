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

	fprintf( dc, "static const uint8_t bitDecodeLUT[] = {\n" );

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
		//printf( "%d %d (%08x)\n", val, last, outcode );
		fprintf( dc, "0x%02x, ", outcode );
	}
	fprintf( dc, "\n};\n\n" );


	fprintf( dc, "static const uint8_t crc_pd_rev[] = {\n" );
	fprintf( dc, "	0b0000, 0b1000, 0b0100, 0b1100,\n" );
	fprintf( dc, "	0b0010, 0b1010, 0b0110, 0b1110,\n" );
	fprintf( dc, "	0b0001, 0b1001, 0b0101, 0b1101,\n" );
	fprintf( dc, "	0b0011, 0b1011, 0b0111, 0b1111 };\n" );

#define CRC_POLY 0x04C11DB7
#define CHECKCRC_VALID 0xc704dd7b

	fprintf( dc, "#define CRC_POLY 0x04C11DB7\n" );
	fprintf( dc, "#define CRC_INIT 0xffffffff\n" );
	fprintf( dc, "#define CHECKCRC_VALID 0xc704dd7b\n" );

	fprintf( dc, "static const uint32_t crc_pd_table[] = {\n" );
	int symbol;
	for( symbol = 0; symbol < 16; symbol++ )
	{

		uint32_t USE_POLY = CRC_POLY;
		uint32_t current_crc = 0;

		{
			uint8_t by = symbol;

			uint32_t newbit, newword;
			uint32_t rl_crc;
			for(int i=0; i<4; i++) {
				newbit = ((current_crc>>31) ^ ((by>>(3-i))&1)) & 1;
				if(newbit) newword=USE_POLY-1; else newword=0;
				rl_crc = (current_crc<<1) | newbit;
				current_crc = rl_crc ^ newword;
			}
		}
		if( (symbol & 3) == 0 ) fprintf( dc, "\t" );
		fprintf( dc, "0x%08x, ", current_crc );
		if( (symbol & 3) == 3 ) fprintf( dc, "\n" );
	}
	fprintf( dc, "};\n" );


	fprintf( dc, "#endif\n" );
}


