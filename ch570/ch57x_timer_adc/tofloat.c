#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main()
{
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
#ifdef SAMP8COMP
		int32_t gv = ((int8_t)getchar());
#else
		if( ( sample & 1 ) == 0 )
		{
			temp = ((uint8_t)getchar());
		}
		uint8_t code = (temp >> 4)&0xf;
		temp <<= 4;
		int32_t gv = 0;
		if( code > 8 )
		{
			gv = 1<<(code - 9);
		}
		else if( code < 8 )
		{
			gv = -1<<(7-code);
		}
#endif

		vs += gv;
		float f = vs / 1024.0f;
		fwrite( &f, 1, 4, fRawData );
		printf( "%f\n", f );
		fflush( stdout );
		sample++;
	} while( 1 );
	fclose( fRawData );
}

