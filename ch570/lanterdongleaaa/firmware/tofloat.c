#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main( int argc, char ** argv )
{
	int mode = 0;

	if( argc == 2 && argv[1][0] == 'B' ) mode = 1;
	if( argc == 2 && argv[1][0] == 'F' )
	{
		FILE * f = fopen( "todump.txt", "rb" );
		char * payload = malloc(1);
		int payloadlen = 0;
		while( !feof( f ) )
		{
			char l[1024];
			char *lp = fgets(l, 1024, f);
			int i;
			for( i = 0; lp[i]; i++ )
			{
				if( lp[i] == ',' ) { lp = lp+i+3; break; }
			}
			if( !lp[i] ) { fprintf( stderr, "Malform\n" ); continue; }

			int c = fgetc(f);
			if( c == EOF )
			{
				payloadlen++;
				payload = realloc( payload, payloadlen );
				payload[payloadlen-1] = 0;
				break;
			}
			payloadlen++;
			payload = realloc( payload, payloadlen );
			payload[payloadlen-1] = c;
		} while( 1 );
		fprintf( stderr, "Readok %d\n", payloadlen );
		printf( payload );
		return 0;
	}

#if 0
{
	int err = 0;
	int value;
	uint32_t temp = 0;;
				int sign = 0;
				if( err < 0 ) { sign = 1; err = -err; }
				if( err == 0 ) { err = 0; value = 8; }
				else
				{
					int negclz = 32 - __builtin_clz(err);
					if( negclz > 7 ) negclz = 7;
					value = negclz;
					err = 1<<(negclz-1);
					if( sign ) { value = 8-negclz; err = -err; }
					else { value = 8+negclz; }
				}				
				temp = (temp<<4) | value;
printf( "value: %d\n", value );
				//if( i&1 ) { senda[i/2] = temp; temp = 0; }
		printf( "%d %d\n", err, temp );
		return 0;
	}
#endif
	uint32_t temp;
	int16_t vs = 0;
	int sample = 0;
	FILE * fRawData = fopen( "audio.dat", "wb" );
	do
	{
//		uint16_t vu = getchar();
//		vu |= getchar()<<8;
//		int16_t vs = vu;

		int32_t gv;
		if( mode == 1 )
		{
			int gc = getchar();
			if( gc == EOF ) break;
			gv = ((int8_t)gc);
			//fprintf( stderr, "%d\n", gv );
		}
		else
		{
			// Wacky 4-bit-per-sample compression
			if( ( sample & 1 ) == 0 )
			{
				int gc = getchar();
				if( gc == EOF ) break;

				temp = ((uint8_t)gc);
			}
			uint8_t code = (temp >> 4)&0xf;
			temp <<= 4;
			gv = 0;
			if( code > 8 )
			{
				gv = 1<<(code - 9);
			}
			else if( code < 8 )
			{
				gv = -1<<(7-code);
			}
		}

		vs += gv;
		float f = vs / 1024.0f;
		fwrite( &f, 1, 4, fRawData );
		printf( "%f\n", f );
		fflush( stdout );
		sample++;
	} while( 1 );
	fclose( fRawData );
}

