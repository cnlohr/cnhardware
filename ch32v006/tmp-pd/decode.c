#include <stdio.h>
#include <stdint.h>
#include "tests.h"


int main()
{
	// Two things to search for:
	//  Pre-Amble: Find some staritng place.
	int i;


	int running = 0;
	int is_decoding_short = 1;

	int last = tests[1][i] > 512;

	for( i = 1; i < 255; i++ )
	{
		int this = tests[1][i] > 512;
		int next = tests[1][i+1] > 512;

		if( this == last) running++;
		else
		{
			printf( "%d\n", running+1 );
			running = 0;
		}

		last = this;
	}

	return 0;
}

