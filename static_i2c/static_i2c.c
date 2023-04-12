// ESP32-S2 demo for scanning an I2C bus.

#define PIN_DIR_OUTPUT GPIO.enable1_w1ts.val
#define PIN_DIR_INPUT GPIO.enable1_w1tc.val
#define PIN_OUT_CLEAR GPIO.out1_w1tc.val
#define PIN_IN GPIO.in1.val

#define _BV(x) (1<<(x))
#define DSDA (42-32)
#define DSCL (41-32)

void i2c_delay( int x ) { int i; for( i = 0; i < 40*x; i++ ) asm volatile( "nop" ); }
#define I2CSPEEDBASE 1
#define I2CDELAY_FUNC(x) i2c_delay(x);
 //esp_rom_delay_us(x*1)

#include "static_i2c.h"


int main()
{
	REG_WRITE( IO_MUX_GPIO41_REG, (1<<FUN_IE_S) | (1<<FUN_PU_S) | ( 1<<FUN_DRV_S) | (1<<MCU_SEL_S) );  // For GPIO41, 42, MCU_SEL needs to be 1.
	REG_WRITE( IO_MUX_GPIO42_REG, (1<<FUN_IE_S) | (1<<FUN_PU_S) | ( 1<<FUN_DRV_S) | (1<<MCU_SEL_S) );
	ConfigI2C();

	while( 1 )
	{
		int i;
		uprintf( "\n	" );
		for( i = 0; i < 16; i++ )
		{
			uprintf( "%c ", (i<10)?('0'+i):('a'+i-10) );
		}

		for( i = 0; i < 128; i++ )
		{
			if( (i & 15) == 0 )
				uprintf( "\n%c ", ((i>>4)<10)?('0'+(i>>4)):('a'+(i>>4)-10) );

			SendStart();
			if( SendByte( i<<1 ) )
			{
				uprintf( "0 " );
			}
			else
			{
				SendByte( 0 );
				SendStart();
				uprintf( "%d ", GetByte( 1 ) );
			}
			SendStop();
		}
		vTaskDelay( 100 );
	}
}
