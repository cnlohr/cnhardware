#include "ch32v003fun.h"
#include <stdio.h>
#include "ch32v307gigabit.h"

int main()
{
	SystemInit();
	funGpioInitAll();


	Delay_Ms(50);
	int r = ch32v307ethInit();
	printf( "R: %d\n",r );
	while(1)
	{
		uint32_t start = SysTick->CNT;
		Delay_Ms( 1000 );
		uint32_t end = SysTick->CNT;
		printf( "%d\n", (int)(end-start) );
	}
}
