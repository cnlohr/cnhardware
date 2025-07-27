/* Small example showing how to use the SWIO programming pin to 
   do printf through the debug interface */

#include "ch32fun.h"
#include <stdio.h>

#define PIN_SCL PA8
#define PIN_SDA PA9
#define DELAY1 ADD_N_NOPS(1); //Delay_Us(1);
#define DELAY2 ADD_N_NOPS(2); //Delay_Us(2);
//#define I2CCLOCKSTRETCHCHECK { for( int to = 256; to >= 0 && funDigitalRead( PIN_SCL ) == 0; to-- ); }
#define DSCL_IHIGH  { funPinMode( PIN_SCL, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SCL, 1 ); } 
#define DSDA_IHIGH  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSDA_INPUT  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSCL_OUTPUT { funDigitalWrite( PIN_SCL, 0 ); funPinMode( PIN_SCL, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define DSDA_OUTPUT { funDigitalWrite( PIN_SDA, 0 ); funPinMode( PIN_SDA, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define READ_DSDA    funDigitalRead( PIN_SDA )
#define I2CNEEDGETBYTE 1
#define I2CNEEDSCAN    1

#define I2CSTATICODE static inline __attribute__((section(".srodata")))

#include "static_i2c.h"

#define AW32001_ADDRESS 0x49
#define LSM6DS3_ADDRESS 0x6a
#define QMI8658_ADDRESS 0x6b

int SetupRegisterMap( int address, const uint8_t * regptr, int regs, const char * name )
{
	int i;
	int fail = 0;
	for( i = 0; i < regs; i++ )
	{
		SendStart();
		int b = SendByte( address<<1 );
		if( b )
		{
			printf( "%s Cannot Configure @ %d\n", name, i );
			fail = 1;
			continue;
		}
		int a = *(regptr++);
		SendByte( a );
		int v = *(regptr++);
		b = SendByte( v );
		SendStop();
		printf( "%s %02xh = %02x [%s]\n", name, a, v, b?"FAIL":"OK" );
		fail |= b;
	}
	return fail;
}

void ProcessQMI8658() __attribute__((section(".srodata")));
void ProcessQMI8658()
{
	int tsamp = 0;

	SendStart();
	int ra = SendByte( QMI8658_ADDRESS<<1 );
	int rb = SendByte( 0x15 );
	SendStart();
	int rc = SendByte( (QMI8658_ADDRESS<<1)|1 );

	uint32_t sw = 0;
	sw = GetByte( 0 );
	sw = GetByte( 0 ) << 8;

	if( sw & 0x4000 )
	{
		printf( "Full SW: %06x [%d %d %d]\n", (int)sw, ra, rb, rc );
		// Need to reset fifo.
		GetByte(1);
		SendStop();

		SetupRegisterMap( LSM6DS3_ADDRESS, 
			(const uint8_t[]){ 0x0a, 0x28, 0x0a, 0x2e },
			2, "LSM6DS3 Overflow" );
		return;
	}

	printf( "%06x ", sw );
	//pattern |= GetByte( 0 ) << 8;

	int samp = 0;
	int samps = (sw & 0x3ff) / 12;

	// practical maximum
	if( samps > 16 ) samps = 16;

	for( samp = 0; samp < samps; samp++ )
	{
		int i;
		for( i = 0; i < 6; i++ )
		{
			uint16_t val = GetByte( 0 );
			int end = (i==5 && samp == samps-1);
			if( end ) printf( "." );
			val |= GetByte( end ) << 8;
			if( samp == 0 )
				printf( "%04x ", val );
		}
		//printf( "\n" );
	}
	printf( "\n" );

	SendStop();
}

void ProcessLSM6DS3() __attribute__((section(".srodata")));
void ProcessLSM6DS3()
{
	int tsamp = 0;

	SendStart();
	int ra = SendByte( LSM6DS3_ADDRESS<<1 );
	int rb = SendByte( 0x3a );
	SendStart();
	int rc = SendByte( (LSM6DS3_ADDRESS<<1)|1 );

	uint32_t sw = 0;
	sw = GetByte( 0 );
	sw |= GetByte( 0 ) << 8ULL;
	sw |= GetByte( 0 ) << 16ULL;

	if( sw & 0x4000 )
	{
		printf( "Full SW: %06x [%d %d %d]\n", (int)sw, ra, rb, rc );
		// Need to reset fifo.
		GetByte(1);
		SendStop();

		SetupRegisterMap( LSM6DS3_ADDRESS, 
			(const uint8_t[]){ 0x0a, 0x28, 0x0a, 0x2e },
			2, "LSM6DS3 Overflow" );
		return;
	}

	printf( "%06x ", sw );
	//pattern |= GetByte( 0 ) << 8;

	int samp = 0;
	int samps = (sw & 0x7ff) / 6;

	// practical maximum
	if( samps > 63 ) samps = 63;

	if( samps == 0 )
		GetByte( 1 ); // Ignore FIFO status 3.
	else
	{
		GetByte( 0 ); // Ignore FIFO status 3.
		GetByte( 0 ); // Ignore FIFO status 4
	}

	for( samp = 0; samp < samps; samp++ )
	{
		int i;
		for( i = 0; i < 6; i++ )
		{
			uint16_t val = GetByte( 0 );
			int end = (i==5 && samp == samps-1);
			if( end ) printf( "." );
			val |= GetByte( end ) << 8;
			if( samp == 0 )
				printf( "%04x ", val );
		}
		//printf( "\n" );
	}
	printf( "\n" );

	SendStop();
}

int main()
{
	int i;
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	ConfigI2C();

	SendStart();
	SendByte( AW32001_ADDRESS<<1 );
	SendByte( 0x06 );
	// Immedaitely set 4.2V vsys, and disable NTE
	SendByte( 0x42 );
	SendByte( 0x30 );
	SendStop();

	// reboot lsm6ds3.
	SetupRegisterMap( LSM6DS3_ADDRESS, (const uint8_t[]){ 0x12, 0x81 }, 1, "LSM6DS3 Reset" ); // Enable special codes and force reset.
	SetupRegisterMap( LSM6DS3_ADDRESS, (const uint8_t[]){ 0x60, 0xb0 }, 1, "QMI8658 Reset" ); // Enable special codes and force reset.

	Delay_Ms(2);


	unsigned start;

	int frameno = 0;

	Scan();

	const static uint8_t AW32001ERegmap[] = {
		0x00, 0x8f, // 00h 4.52V, 500mA
		0x01, 0x2c, // 01h Disconnect after **8s with INT low**, 4s auto-on, charge disable, 2.76v UVLO
		0x02, 0x0f, // 02h 128mA charge.
		0x03, 0x91, // 03h 2A discharge, 3mA termination
		0x04, 0xa3, // 04h 4.2V Target Voltage
		0x05, 0x7a, // 05h Watchdog disable, 160s watchdog, termation allowed, safety enabled, fast charge timer 5h, termation timer disable.
		0x06, 0x42, // 06h **NTC DISABLE**, 2x extended safety, FET_DIS=0 = enabled. **NTC_INT disable.**
		0x07, 0x30, // 07h **4.2V VSYS** Enable PCB OTP, VIN_DPM loop enabled, 120C TJ_Reg
		0x08, 0x40, // 08h (System Status)
		0x09, 0x00, // 09h Enter Shipping Mode Deglitch Time 1s. Also, Fault Status
		0x0a, 0x49, // 0ah Chip Identification (0x49)
		0x0b, 0x43, // 0bh ICHG default, IPRE default, VIN plug no-ship: 100ms
		0x0c, 0x10, // 0ch Pre-charge, delay, etc.
		0x22, 0x0b, // 22h **100ms to exit shipping
	};

	SetupRegisterMap( AW32001_ADDRESS, AW32001ERegmap, sizeof(AW32001ERegmap)/2, "AW32001E" );

	const static uint8_t LSM6DS3Regmap[] = {
		0x12, 0x44, // CTRL3_C - unset reboot. + BDU
		0x10, 0x5a, // CTRL1_XL - 208Hz, +/-8g
		0x11, 0x54, // CTRL2_G - 208Hz, 1000dps

		0x0a, 0x28, // FIFO_CTRL5 - Disable FIFO (will re-enable)
		0x06, 0x60, // FIFO_CTRL1 - FIFO size.
		0x07, 0x00, // FIFO_CTRL2 - No temperature in FIFO.
		0x08, 0x09, // FIFO_CTRL3 - Put accel+gyro in FIFO.
		0x09, 0x00, // FIFO_CTRL4 - No decimation
		0x0a, 0x2b, // FIFO_CTRL5 - 208Hz, Continuous mode FIFO. (was 0x2e) - if 12.5Hz (set to 0x0e)
		0x13, 0x00, // CTRL4_C - No extra stuff, don't stop on fth.
		0x15, 0x10, // CTRL6_C - High Performance.
		0x16, 0x00, // CTRL7_C - Just default settings.
		0x13, 0x01, // CTRL4_C - Stop on fth
	};

	SetupRegisterMap( LSM6DS3_ADDRESS, LSM6DS3Regmap, sizeof(LSM6DS3Regmap)/2, "LSM6DS3" );


	const static uint8_t QMI8658Regmap[] = {
		0x02, 0x61, // CTRL1 = Enable
		0x03, 0x25, // CTLR2 = 8G's, 250 (235) Hz
		0x04, 0x35, // CTRL3 = 512dps, 235Hz

		0x06, 0x00, // Disable LPF
		0x07, 0x00, // No motion detection

		0x08, 0x30, // high speed, enable gyro, accel.
		0x13, 0x80, // FIFO HWM
		0x14, 0x0d, // 128 samples, FIFO mode.
	};
	SetupRegisterMap( QMI8658_ADDRESS, QMI8658Regmap, sizeof(QMI8658Regmap)/2, "QMI8658" );

/*
	Delay_Ms(6000);

	// This is how you shut down the AW32001.
	SendStart();
	SendByte( AW32001_ADDRESS<<1 );
	SendByte( 0x06 );
	SendByte( 0x62 ); // FET_DIS
*/
	while(1)
	{
		frameno++;

		//ProcessLSM6DS3();
		ProcessQMI8658();

#if 1
		// Monitor a bunch of registers.
		printf( "  " ); for( i = 0; i < 16; i++ ) printf( "  %x", i );
		for( i = 0; i < 0x80; i++ )
		{
			int addy = QMI8658_ADDRESS;
			if( ( i & 0xf ) == 0 ) printf( "\n%02x ", i );
			SendStart();
			int b = SendByte( addy<<1 );
			SendByte( i );
			SendStart();
			SendByte( (addy<<1) | 1 );
			int v = GetByte( 1 );
			SendStop();
			if( 0 && b )
				printf( "xx " );
			else
				printf( "%02x ", v );
		}
		printf( "\n" );
#endif

		//Delay_Ms(400);
	}
}
