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
	R32_PA_SET = SWD_IO;
	R32_PA_DIR = SWC_IO | SWD_IO | POWER_IO; // Drive out.

	R32_PA_SET = SWD_IO;
	R32_PA_CLR = SWD_IO;
	R32_PA_SET = SWD_IO;
	R32_PA_CLR = SWD_IO;
	R32_PA_SET = SWD_IO;
	R32_PA_CLR = SWD_IO;
	ADD_N_NOPS(4);
	R32_PA_SET = SWD_IO;
	R32_PA_CLR = SWD_IO;
	ADD_N_NOPS(4);
	R32_PA_DIR &= ~SWD_IO;


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

