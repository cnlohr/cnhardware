#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	int i;
	int v = 0;
	#define APRL 127
	#define NUPHC 256

	uint8_t phain[NUPHC];
	uint8_t mid[NUPHC];
	uint8_t orig[NUPHC];

	int debug[NUPHC];

	for( i = 0; i < NUPHC; i++ )
	{
		v += (rand()%APRL)-(APRL/2);
		orig[i] = phain[i] = v;
	}

	int iter;
	for( iter = 0; iter < 20; iter++ )
	{
		for( i = 0; i < NUPHC; i++ )
		{
			int ths = phain[i];
			int prev = phain[(i-1+NUPHC)%NUPHC];
			int next = phain[(i+1)%NUPHC];

			if( (prev - ths) > 127 ) prev -=256;
			if( (prev - ths) <-127 ) prev +=256;
			if( (next - ths) > 127 ) next -=256;
			if( (next - ths) <-127 ) next +=256;
			debug[i] = next;

			float f = prev * 0.25 + ths * .5 + next * 0.25;
			int o = f;
			mid[i] = (uint8_t)o;
		}
		memcpy( phain, mid, sizeof(mid) );
	}

	FILE * phases = fopen( "rainbowphase.h", "w" );
	fprintf( phases, "const uint8_t rainbowphase[%d] = {", NUPHC );
	for( i = 0; i < NUPHC; i++ )
	{
		fprintf( phases, "%s0x%02x, ", ((i&0xf) == 0 ) ?"\n\t" : "", phain[i] );
		//printf( "%d,%d,%d\n", orig[i], phain[i],debug[i] );
	}
	fprintf( phases, "};\n" );

}

