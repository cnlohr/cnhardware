#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main()
{
	int16_t vs = 0;
	FILE * fRawData = fopen( "audio.dat", "wb" );
	do
	{
//		uint16_t vu = getchar();
//		vu |= getchar()<<8;
//		int16_t vs = vu;
		vs += ((int8_t)getchar());


		float f = (vs*4) / 16384.0f;

		fwrite( &f, 1, 4, fRawData );
		printf( "%f\n", f );
		fflush( stdout );
	} while( 1 );
	fclose( fRawData );
}

