#include "ch32fun.h"
#include <stdio.h>

#define RANDOM_STRENGTH 2

#include "lib_rand.h"

// TODO: Change access address

#define ACCESS_ADDRESS 0x8E89BED6 // the "BED6" address for BLE advertisements
#define PHY_MODE PHY_1M

#include "iSLER.h"

#define SCANCHAN 5
uint32_t scanplan[SCANCHAN] = { 0, 8, 17, 27, 36 };

void RunFunction() __HIGH_CODE;

void RunFunction()
{
	uint32_t scanpos = 0;

	#define HOPTIMEOUT 16 * DELAY_MS_TIME

	uint32_t now = funSysTick32();
	uint32_t nextSend = now + 10000 * DELAY_US_TIME;
	uint32_t hopTimeOutAt = now;
	uint32_t channel = scanplan[0];
	uint32_t rxthis = 0;

	while(1)
	{
		now = funSysTick32();
		if( TimeElapsed32( now, hopTimeOutAt ) )
		{
			if( !rxthis ) printf( "-" );
			if( scanpos != 0 ) printf( "," );
			scanpos++;
			if( scanpos >= SCANCHAN ) scanpos = 0;
			if( scanpos == 0 ) printf( "\n" );
			channel = scanplan[scanpos];
			nextSend = now + ((rand()&0xfff) + 10000) * DELAY_US_TIME; //10-14ms
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);
			rxthis = 0;
		}

		if( rx_ready )
		{
			uint8_t *frame = (uint8_t*)LLE_BUF;
			int rssi = iSLERRSSI();

			if( iSLERCRCOK() )
			{
				//printf( "%d [%02x %02x %02x %02x %02x %02x]\n", rssi, frame[0], frame[1], frame[2], frame[3], frame[4], frame[5] );
				if( frame[2] == 0x66 && frame[3] == 0x55 && frame[4] == 0x44 )
				{
					if( !rxthis ) printf( "%d", rssi );
					rxthis = 1;
				}

				hopTimeOutAt = now + 8000; // Got Packet.  Set planned jump.
			}
			else
			{
				//printf( "x" );
			}
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);
		}

		if( TimeElapsed32( funSysTick32(), nextSend ) > 0 )
		{
			//printf( "." );
			nextSend += DELAY_MS_TIME * 10;

			static uint8_t txbuf[] = {
					0x02, 0x0d, // header for LL: PDU + frame length
					0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
					0x06, 0x09, 'R', 'X', ':', '?', '?'}; // 0x09: "Complete Local Name"

			iSLERTX(ACCESS_ADDRESS, txbuf, sizeof(txbuf), channel, PHY_MODE);
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);

			hopTimeOutAt = now + 8000;
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
