#include <stdio.h>

int main()
{
	// Two things to search for:
	//  Pre-Amble: Find some staritng place.
	int i;


	int last = tests[0][i] > 512;

	int running = 0;
	int is_decoding_short = 1;

	for( i = 1; i < 127; i++ )
	{
		int this = tests[0][i] > 512;
		int next = tests[0][i+1] > 512;

		if( this == last) running++;


		last = this;
	}

	return 0;
}

