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
	printf( "%02x:%02x:%02x:%02x:%02x:%02x\n", ch32v307eth_mac[0], ch32v307eth_mac[1], ch32v307eth_mac[2], ch32v307eth_mac[3], ch32v307eth_mac[4], ch32v307eth_mac[5] );
	Delay_Ms(3000);
	while(1)
	{
		Delay_Ms( 50 );

		uint8_t testframe[] = { 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
			0x02, 0xcd, 0xef, 0x12, 0x34, 0x56, // Source
			0x08, 0x00, // IP
			0x45, 0x00, // IP version + ToS
			0x00, 0x32, // 50-byte full payload.
			0x00, 0x00, // Identification
			0x40, 0x00, // Flags (Don't fragment) and offset.
			0x40, //TTL
			0x11, // UDP
			0x00, 0x00, // Header Checksum
			0x01, 0x02, 0x03, 0x04, // Source Address
			0xff, 0xff, 0xff, 0xff, // Destionation Address
			0x04, 0x01, // Port 1025 Source
			0x04, 0x02, // Port 1026 Destination
			0x00, 0x1E, // 30-byte payload
			0x00, 0x00, // Checksum
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		};
		testframe[sizeof(testframe)-1]++;
		int r = ch32v307ethTransmitStatic(testframe, sizeof(testframe) );
		PrintCurrentPHYState();
		printf( "%08lx // %08lx -> %d\n", ETH->DMASR, ETH->DMAOMR, r );

	}
}
