#include <stdio.h>
#include <stdlib.h>

int main()
{
	FILE * fRawData = fopen( "audio.dat", "wb" );
	char buffer[1024];
	while( fgets( buffer, 1023, stdin ) != 0 )
	{
		int num = atoi( buffer );
		float f = num / 1600.0f;
		fwrite( &f, 1, 4, fRawData );
//		printf( "%f\n", f );
	}
	fclose( fRawData );
}

