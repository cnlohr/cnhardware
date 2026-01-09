#ifndef EMU
#include "ch32v003fun.h"
#else
#include "emu.h"
#endif
#include <stdio.h>
#include <string.h>

#include "graphics.h"

#include "games.h"

#ifndef EMU
void WriteHighScore( int32_t hs )
{
	// Unkock flash - be aware you need extra stuff for the bootloader.
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;

	// For unlocking programming, in general.
	FLASH->MODEKEYR = 0x45670123;
	FLASH->MODEKEYR = 0xCDEF89AB;

	//Erase Page
	FLASH->CTLR = CR_PAGE_ER;
	FLASH->ADDR = (intptr_t)highScorePtr & 0xffffffc0;
	FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
	while( FLASH->STATR & FLASH_STATR_BSY );  // Takes about 3ms.

	// Clear buffer and prep for flashing.
	FLASH->CTLR = CR_PAGE_PG;  // synonym of FTPG.
	FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
	FLASH->ADDR = (intptr_t)highScorePtr;  // This can actually happen about anywhere toward the end here.

	while( FLASH->STATR & FLASH_STATR_BSY );  // No real need for this.

	*highScorePtr = hs; //Write to the memory
	FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD; // Load the buffer.
	while( FLASH->STATR & FLASH_STATR_BSY );  // Only needed if running from RAM.

	// Actually write the flash out. (Takes about 3ms)
	FLASH->CTLR = CR_PAGE_PG|CR_STRT_Set;

	while( FLASH->STATR & FLASH_STATR_BSY );
}
#endif

uint32_t modeStartTime;;
int interstitial = 0;
int freebieID = 0;


#ifdef WASM
int __attribute__((export_name("main"))) main()
#else
int main()
#endif
{
	SystemInit();

	funGpioInitAll();
	funPinMode( PD0, GPIO_CFGLR_OUT_50Mhz_PP );
	funDigitalWrite( PD0, FUN_LOW );

	// button setup
	funPinMode( PA1, GPIO_CFGLR_IN_PUPD );
	funPinMode( PA2, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PA1, FUN_HIGH );
	funDigitalWrite( PA2, FUN_HIGH );

	ssd1306_rst();
	if(ssd1306_spi_init())
	{
		// Could not init OLED.  Cry?
	}
	ssd1306_init();

	ssd1306_cmd( SSD1306_SETMULTIPLEX );
	ssd1306_cmd( 39 );
	ssd1306_cmd( SSD1306_SETDISPLAYOFFSET );
	ssd1306_cmd( 0 );

	ssd1306_cmd( SSD1306_SETPRECHARGE ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETCONTRAST ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETVCOMDETECT ); ssd1306_cmd( 0x40 );
	ssd1306_cmd( 0xad ); ssd1306_cmd( 0x90 ); // Set Charge pump (set to 0x90 for extra bright)

	gameTimeUs = 0;
	gameMode = GameModeMainMenu;

	modeStartTime = SysTick->CNT;
	while(1)
	{
		if( gameModeForce )
		{
			gameMode = gameModeForce;
			gameModeID = gameModeForceID;
		}

		int ret = gameMode();
		uint32_t now = SysTick->CNT;

		int thisMask = (( GPIOA->INDR >> 1 ) & 3) ^ 3;
		buttonEventDown = thisMask & ~buttonMask;
		buttonEventUp = ~thisMask & buttonMask;
		buttonMask = thisMask;

		gameTimeUs = (now - modeStartTime) / 6; // This actually invokes software divide even though it's not needed.
		int gameTimeUsFlip = (now - modeStartTime) < 0;
		if( gameTimeUsFlip ) gameTimeUs += 715827883;
		swapBuffer();

#ifdef EMU
		EmuUpdate();
#endif

		frameno++;

		if( ret != 0 )
		{
			glyphdraw_invert = 0;
			glyphdraw_nomask = 0;
			gameTimeUs = 0;
			modeStartTime = now;
			frameno = 0;
			memset( gameData, 0, sizeof( gameData ) );

			if( !interstitial )
				gameNumber++;

			if( gameNumber == maxGames+1 )
			{
				gameMode = GameModeEnding;
			}
			else if( gameNumber == maxGames+2 )
			{
				gameNumber = 0;
				gameMode = GameModeMainMenu;
			}
			else
			{
				_rand_lfsr = SysTick->CNT;
				if( gameNumber == 1 )
				{
					// Decide how the set of games will go.
					freebieID = ( _rand_gen_32b() % (maxGames-1) ) + 2;
					memset( gamePlayedCount, 0, sizeof( gamePlayedCount ) );
					totalScore = 0;
				}
				if( interstitial || gameNumber == 1 )
				{
					if( gameNumber == freebieID )
					{
						int index = (_rand_gen_32b()) % (sizeof(gameModesFree)/sizeof(gameModesFree[0]));
						gameMode = gameModesFree[ index ];
						gameModeID = gameModeFreeIDs[ index ];
					}
					else
					{

						int zeroleft = 0;
						int i;
						for( i = 0; i < sizeof( gamePlayedCount ) / sizeof( gamePlayedCount[0] ); i++ )
						{
							if( gamePlayedCount[i] == 0 )
								zeroleft++;
						}

						int index;
						do
						{
							index = (_rand_gen_32b()) % (sizeof(gameModes)/sizeof(gameModes[0]));

							// If a category has zero left, then make sure we select one of those.
							if( zeroleft )
							{
								if( gamePlayedCount[index] == 0 ) break;
							}
							else
							{
								break;
							}
						} while( 1 );

						gameMode = gameModes[ index ];
						gameModeID = gameModeIDs[ index ];
						gamePlayedCount[ index ]++;
					}
					interstitial = 0;
				}
				else
				{
					gameMode = GameModeInterstitial;
					interstitial = 1;
				}
			}
		}
	}
	return 0;
}


