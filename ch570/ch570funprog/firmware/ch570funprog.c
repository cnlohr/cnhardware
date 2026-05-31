#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "fsusb.h"

#define LED PA7
#define LED_ON 0


#define POWER_IO    PA7
#define SWC_IO      PA3
#define SWD_IO      PA2

uint32_t count;
int doreboot = 0;
int last = 0;

void handle_debug_input( int numbytes, uint8_t * data )
{
	last = data[0];
	count += numbytes;
}

int lrx = 0;
uint8_t scratchpad[256];

int HandleHidUserSetReportSetup( struct _USBState * ctx, tusb_control_request_t * req )
{
	int id = req->wValue & 0xff;
	if( id == 0xaa && req->wLength <= sizeof(scratchpad) )
	{
		ctx->pCtrlPayloadPtr = scratchpad;
		lrx = req->wLength;
		return req->wLength;
	}
	return 0;
}

int HandleHidUserGetReportSetup( struct _USBState * ctx, tusb_control_request_t * req )
{
	int id = req->wValue & 0xff;
	switch( id )
	{
	case 0xaa:
	{
		ctx->pCtrlPayloadPtr = scratchpad;
		if( sizeof(scratchpad) - 1 < lrx )
			return sizeof(scratchpad) - 1;
		else
			return lrx;
	}
	case 0xe2: // Copy the printf debug buffer out of DMDATA0.
		memcpy( scratchpad, (char*)DMDATA0, 8 );
		ctx->pCtrlPayloadPtr = scratchpad;
		*DMDATA0 = 0;
		return 8;
	}
	return 0;
}

void HandleHidUserReportDataOut( struct _USBState * ctx, uint8_t * data, int len )
{
	switch( data[0] )
	{
	case 0xe1:
		if( len > 7 )
		{
			if( strncmp( (char*)(data+1), "\xbe\xef\x00\xc0\x01\xd0\x0d", 7 ) == 0 )
			{
				doreboot = 1000;
			}
		}
		break;
	}
}

int HandleHidUserReportDataIn( struct _USBState * ctx, uint8_t * data, int len )
{
	return len;
}

void HandleHidUserReportOutComplete( struct _USBState * ctx )
{
	return;
}


uint32_t dmaa[] = {
		0x010203, 0x010203, 0x010203, 0x010203, 0x010203
	};

__HIGH_CODE
void ASetup()
{

	SYS_SAFE_ACCESS( R32_PA_DIR = SWC_IO | SWD_IO | POWER_IO; ); // Drive out.
//	R32_PA_PD_DRV = SWD_IO | SWC_IO; // 20mA
	SYS_SAFE_ACCESS( R8_TMR_CTRL_MOD = 0b00000010; );

	SYS_SAFE_ACCESS( R8_PWM_OUT_EN = RB_PWM2_OUT_EN | RB_PWM3_OUT_EN; );
	SYS_SAFE_ACCESS( R8_PWM_CONFIG = 0; ); // 8-bit mode.
	SYS_SAFE_ACCESS( R32_TMR_CNT_END = 31; );

	SYS_SAFE_ACCESS( R32_PWM_DMA_BEG = dmaa; );
	SYS_SAFE_ACCESS( R32_PWM_DMA_END = ((uint8_t*)dmaa) + sizeof( dmaa ); );
//	SYS_SAFE_ACCESS( R8_PWM_DMA_CTRL = 5; );
	SYS_SAFE_ACCESS( R8_PWM_DMA_CTRL = 7; );

	// 8.3.2 PWM Function
	SYS_SAFE_ACCESS( R32_PWM1_3_DATA = (4<<8) | (4<<16) | 7; );

//	SYS_SAFE_ACCESS( R16_PWM_CYC_VALUE = (4<<8) | (4<<16) | 1; ); // Not used in 8-bit mode.

	SYS_SAFE_ACCESS( R16_PWM_CLOCK_DIV = 16; );
	SYS_SAFE_ACCESS( R8_PWM_POLAR = RB_PWM3_POLAR; );

/*	SYS_SAFE_ACCESS( R32_TMR_FIFO = 0x0309; );
	SYS_SAFE_ACCESS( R32_TMR_FIFO = 0x0309; );
	SYS_SAFE_ACCESS( R32_TMR_FIFO = 0x0309; );
	SYS_SAFE_ACCESS( R32_TMR_FIFO = 0x0309; );
*/
//	SYS_SAFE_ACCESS( R32_TMR_FIFO = 20; );
//	SYS_SAFE_ACCESS( R32_TMR_FIFO = 20; );
	SYS_SAFE_ACCESS( R8_TMR_CTRL_MOD = 0b00001100; );

printf( "%08x\n", R32_PA_DIR );
//	funDigitalWrite( SWD_IO, !LED_ON );

//	SYS_SAFE_ACCESS( R8_SLP_CLK_OFF1=2; );
//	SYS_SAFE_ACCESS( R8_SLP_CLK_OFF0=(1<<4); );

	//R8_PWM_CONFIG = 0b100 | RB_PWM_CYCLE_SEL;

/*	*((volatile uint16_t *)((&R8_PWM2_DATA))) = 12;
	*((volatile uint16_t *)((&R8_PWM3_DATA))) = 12;

	*((volatile uint16_t *)((&R16_PWM2_DATA))) = 12;
	*((volatile uint16_t *)((&R16_PWM3_DATA))) = 12;
*/

//	SYS_SAFE_ACCESS( R8_PWM_CONFIG |= RB_PWM_SYNC_EN; );
//	SYS_SAFE_ACCESS( R8_PWM_CONFIG |= RB_PWM_SYNC_START; );

//	SYS_SAFE_ACCESS( R32_PA_OUT = 0; );
//	SYS_SAFE_ACCESS( R32_PA_OUT = SWC_IO | SWD_IO | POWER_IO; );
}

__USBFS_FUN_ATTRIBUTE
static __attribute__((noreturn)) void processLoop()
{
	while(1)
	{
		//printf( "%lu %08lx %lu %d %d\n", USBDEBUG0, USBDEBUG1, USBDEBUG2, 0, 0 );
		if( doreboot )
		{
			if( --doreboot == 0 )
			{
#if defined(CH5xx)
				USBFSReset();
				jump_isprom();
#else
				// There aren't any other chips that can reboot into USB bootloader, are there?
#endif
			}
		}
		printf( "loop\n" );

		ASetup();
	}
}



int main()
{
	SystemInit();

	funGpioInitAll();

	funPinMode( LED, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( LED, !LED_ON );

	printf("USBFS starting...");

	USBFSSetup();

	printf("ok\n");

	funDigitalWrite( LED, LED_ON );

	processLoop();
}

