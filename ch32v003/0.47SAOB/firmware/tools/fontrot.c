#include <stdio.h>
#include <stdint.h>
#include "../font8x8_basic.h"


/* ------------------------- transpose8rS32 ------------------------- */
// https://github.com/hcs0/Hackers-Delight/blob/master/transpose8.c.txt
/* This is transpose8r32 but using the GLS method of bit field swapping.
   Instruction counts for the calculation part:
    8 ANDs
   12 XORs
   10 shifts
    2 ORs
    5 mask generation
   --
   37 total (recursive method using GLS's bit swapping) */

void transpose8rS32(unsigned char A[8], int m, int n,
                 unsigned char B[8]) {
   unsigned x, y, t;

   // Load the array and pack it into x and y.

   x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
   y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];

   t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
   t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);

   t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
   t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);

   t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
   y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
   x = t;

   B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x;
   B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y;
}


// Reversing bits in a word, basic interchange scheme.
uint8_t rev1(uint8_t x) {
   x = (x & 0x55) <<  1 | (x & 0xAA) >>  1;
   x = (x & 0x33) <<  2 | (x & 0xCC) >>  2;
   x = (x & 0x0F) <<  4 | (x & 0xF0) >>  4;
   return x;
}


void exchange( uint8_t * a, uint8_t * b )
{
	*a = *a ^ *b;
	*b = *a ^ *b;
	*a = *a ^ *b;
}


int main()
{
	int i;
	printf( "static const unsigned char font8x8_basic_trunrot[96][8] = {\n" );
	for( i = 32; i < 128; i++ )
	{
		unsigned char rotated[8];
		transpose8rS32( font8x8_basic[i], 1, 1, rotated );

		int j;
		for( j = 0; j < 8; j++ ) rotated[j] = rev1(rotated[j] );

		for( j = 0; j < 4; j++ ) exchange( &rotated[7-j], &rotated[j] );


		printf( "\t{ 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, }, // U+%02x %c\n", 
			rotated[0], rotated[1], rotated[2], rotated[3], rotated[4], rotated[5], rotated[6], rotated[7], i, i );
	}
	printf( "};\n" );
}
