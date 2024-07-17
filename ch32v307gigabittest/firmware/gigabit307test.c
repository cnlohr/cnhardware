#include "ch32v003fun.h"
#include <stdio.h>

int main()
{
	SystemInit();
	funGpioInitAll();

//	printf( "!!!!!!!\n" );

	while(1)
	{
		printf( "AAA\n" );
		Delay_Ms( 10 );
		Delay_Ms( 10 );
	}
}
