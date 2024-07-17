#include "ch32v003fun.h"
#include "ch32v307gigabit.h"
#include <stdio.h>

int main()
{
	SystemInit();
	funGpioInitAll();


	Delay_Ms(50);
	int r = ch32v307ethInit();
	printf( "R: %d\n",r );
	while(1)
	{
		printf( "AAA\n" );
		Delay_Ms( 10 );
		Delay_Ms( 10 );
	}
}
