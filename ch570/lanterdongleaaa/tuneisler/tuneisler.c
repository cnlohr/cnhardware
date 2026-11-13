#include "ch32fun.h"
#include <stdio.h>

#define ACCESS_ADDRESS 0x8E89BED6 // the "BED6" address for BLE advertisements
#define PHY_MODE PHY_1M

#include "iSLER.h"

#define CHANNEL 38
void RunFunction() __HIGH_CODE;

void RunFunction()
{
	uint32_t nextSend = funSysTick32();

	iSLERRX(ACCESS_ADDRESS, CHANNEL, PHY_MODE);

	while(1)
	{
		if( rx_ready )
		{
			uint8_t *frame = (uint8_t*)LLE_BUF;
			int rssi = iSLERRSSI();

			if( iSLERCRCOK() )
			{
				//printf( "%d [%02x %02x %02x %02x %02x %02x]\n", rssi, frame[0], frame[1], frame[2], frame[3], frame[4], frame[5] );
				if( frame[2] == 0x66 && frame[3] == 0x55 && frame[4] == 0x44 )
				{
					printf( "%d\n", rssi );
				}
			}
			else
			{
				//printf( "x" );
			}
			iSLERRX(ACCESS_ADDRESS, CHANNEL, PHY_MODE);
		}

		if( TimeElapsed32( funSysTick32(), nextSend ) > 0 )
		{
			//printf( "." );
			nextSend += DELAY_MS_TIME * 10;


			static uint8_t txbuf[] = {
					0x02, 0x0d, // header for LL: PDU + frame length
					0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
					0x06, 0x09, 'R', 'X', ':', '?', '?'}; // 0x09: "Complete Local Name"

			iSLERTX(ACCESS_ADDRESS, txbuf, sizeof(txbuf), 37, PHY_MODE);
			iSLERRX(ACCESS_ADDRESS, CHANNEL, PHY_MODE);
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
