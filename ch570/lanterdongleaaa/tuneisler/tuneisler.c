#include "ch32fun.h"
#include <stdio.h>

#define ACCESS_ADDRESS 0x8E89BED6 // the "BED6" address for BLE advertisements
#define PHY_MODE PHY_1M

#include "iSLER.h"

//void RunFunction() __HIGH_CODE;

void RunFunction()
{
	uint32_t nextSend = funSysTick32();

	while(1)
	{
		iSLERRX(ACCESS_ADDRESS, 37, PHY_MODE);
		if( rx_ready )
		{
			uint8_t *frame = (uint8_t*)LLE_BUF;
			int rssi = iSLERRSSI();

			if( !iSLERCRCOK() ) { printf( "X\n" ); return; }

			printf( "%d\n", rssi );
		}

		if( TimeElapsed32( funSysTick32(), nextSend ) > 0 )
		{
			printf( "." );
			nextSend += DELAY_MS_TIME * 10;


			const uint8_t txbuf[] = {
					0x02, 0x0d, // header for LL: PDU + frame length
					0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
					0x06, 0x09, 'R', 'X', ':', '?', '?'}; // 0x09: "Complete Local Name"

			iSLERTX(ACCESS_ADDRESS, txbuf, sizeof(txbuf), 37, PHY_MODE);
		}
	}
}

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	printf( "Starting\n" );
	iSLERInit(LL_TX_POWER_0_DBM);


	RunFunction();
}
