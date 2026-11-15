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
	uint32_t hopTimeOutAtNoPacket = now;
	uint32_t channel = scanplan[0];
	uint32_t rxthis = 0;
	uint32_t txalready = 0;
	uint32_t subsend = 0;

	while(1)
	{
		now = funSysTick32();

		int timeoutIfGotPacket = TimeElapsed32( now, hopTimeOutAt );
		int timeoutIfNotGotPacket = TimeElapsed32( now, hopTimeOutAtNoPacket );
		int timeout = rxthis ? timeoutIfGotPacket : timeoutIfNotGotPacket;
		if( timeout > 0 )
		{
//printf( "%d %d   %d\n", now, hopTimeOutAt, TimeElapsed32( now, hopTimeOutAt ) );
			if( !rxthis ) printf( "-" );
			scanpos++;
			if( scanpos >= SCANCHAN ) scanpos = 0; else printf( "," );
			if( scanpos == 0 ) printf( "\n" );
			channel = scanplan[scanpos];
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);
			rx_ready = 0;
			nextSend = now + ((rand()&0xfff) + 10000) * DELAY_US_TIME; //10-14ms

			//iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);
			rxthis = 0;
			txalready = 0;
			subsend = 0;

			hopTimeOutAt = now + 20000 * DELAY_US_TIME; // Always stable by default.
			hopTimeOutAtNoPacket = hopTimeOutAt + ((rand()&0xfff) + 8000) * DELAY_US_TIME; //8-12ms longer
		}

		if( rx_ready )
		{
			uint8_t *frame = (uint8_t*)LLE_BUF;
			int rssi = iSLERRSSI();

			if( iSLERCRCOK() )
			{
				if( frame[2] == 0x66 && frame[3] == 0x55 && frame[4] == 0x44 )
				{
					//printf( "%d [%02x %02x %02x %02x %02x %02x]\n", rssi, frame[0], frame[1], frame[2], frame[3], frame[4], frame[5] );
					if( !rxthis ) printf( "%d", rssi );
					rxthis = 1;

					int32_t projExp = *((uint32_t*)&frame[8]) - (300*DELAY_US_TIME /*Assume 2ms ping*/);
					int32_t nowRemain = TimeElapsed32( hopTimeOutAt, now );
					uint32_t targetRemain = (nowRemain * 3)/4 + (projExp/4);

					hopTimeOutAt = targetRemain + now;
					//printf( "[%d %d %d  %d]\n", projExp, nowRemain, targetRemain, timeout );
				}
			}
			else
			{
				//printf( "x" );
			}
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);
		}

		if( TimeElapsed32( now, nextSend ) > 0 )
		{
			static uint8_t txbuf[] = {
					0x02, 0x0d, // header for LL: PDU + frame length
					0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
					0xaa, 0xaa, 0xaa, 0xaa, ':', '?', '?'}; // 0x09: "Complete Local Name"

			uint32_t projExp = TimeElapsed32( hopTimeOutAt, now ); // eg 132731, ideal timeout (assuming we got a packet)
			*((uint32_t*)&txbuf[8]) = projExp;

			iSLERTX(ACCESS_ADDRESS, txbuf, sizeof(txbuf), channel, PHY_MODE);
			iSLERRX(ACCESS_ADDRESS, channel, PHY_MODE);

			if( subsend == 3 )
			{
				nextSend = now + 0x7fffffff; // Far future (Will never be hit)
			}
			else
			{
				nextSend = ((rand()&0x3ff)+100) * DELAY_US_TIME; // Random future (.1 to 1.1ms)
				subsend++;
			}
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
