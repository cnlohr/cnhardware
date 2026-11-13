#include "ch32fun.h"
#include <stdio.h>

#define ACCESS_ADDRESS 0x8E89BED6 // the "BED6" address for BLE advertisements

#include "iSLER.h"

void RunFunction() __HIGH_CODE;

void RunFunction()
{
	uint32_t nextSend = funSysTick32();

	while(1)
	{
		iSLERRX(ACCESS_ADDRESS, 38, PHY_1M);
		if( rx_ready )
		{
			if( !iSLERCRCOK() ) return;

			uint8_t *frame = (uint8_t*)LLE_BUF;
			int rssi = iSLERRSSI();

			printf( "%d\n", rssi );
		}

		if( TimeElapsed32( funSysTick32(), nextSend ) > 0 )
		{
			nextSend += DELAY_MS_TIME * 10;

			uint8_t txbuf[8] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x12, 0x34 };
			iSLERTX(ACCESS_ADDRESS, txbuf, sizeof(txbuf), 38, PHY_1M);
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
