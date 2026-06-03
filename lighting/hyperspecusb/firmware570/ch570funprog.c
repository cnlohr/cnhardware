#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "fsusb.h"


#define PIN_SWD PA2
#define PIN_SWC PA4

#define LED PA5
#define LED_ON 0
#define PIN_TARGETPOWER PA5

#define POWER_ON 0
#define POWER_OFF 1
#define PIN_TARGETPOWER_MODE GPIO_CFGLR_OUT_10Mhz_PP



// Allow reading and writing to the scratchpad via HID control messages.
__attribute__ ((aligned(4))) uint8_t scratch[264];
__attribute__ ((aligned(4))) uint8_t retbuff[264];

volatile uint32_t usb_pending = 0;
volatile uint32_t scratch_return = 0;

#include "the_programmer_from_ch32fun_rvswdio.h"

int doreboot = 0;
uint32_t rebootat = 0;

int didnak;

int missed, hit;

int HandleHidUserSetReportSetup( struct _USBState * ctx, tusb_control_request_t * req )
{
	int id = req->wValue & 0xff;
	if( ( id >= 0xaa && id <= 0xae ) && req->wLength <= sizeof(scratch) )
	{
		if( usb_pending ) return 0;
		ctx->pCtrlPayloadPtr = scratch;
		usb_pending = req->wLength;
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
	case 0xab:
	case 0xac:
	case 0xad:
	case 0xae:
	{
		if( scratch_return )
		{
			hit++;
			scratch_return = 0;
			ctx->pCtrlPayloadPtr = retbuff;
			if( sizeof(retbuff) - 1 < req->wLength )
				return sizeof(retbuff) - 1;
			else
				return req->wLength;
		}
		else
		{
			missed++;
			didnak = req->wLength;
			return 0;
		}
	}
	case 0xe2: // Copy the printf debug buffer out of DMDATA0.
		memcpy( scratch, (char*)DMDATA0, 8 );
		ctx->pCtrlPayloadPtr = scratch;
		*DMDATA0 = 0;
		return 8;
	}
	return -1;
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
				doreboot = 2;
				rebootat = SysTick->CNTL + 1000000;
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


__USBFS_FUN_ATTRIBUTE
static __attribute__((noreturn)) void processLoop()
{
	while(1)
	{
		//printf( "%lu %08lx %lu %d %d\n", USBDEBUG0, USBDEBUG1, USBDEBUG2, 0, 0 );
		if( doreboot )
		{
			if( (int)(SysTick->CNTL - rebootat) > 0 )
			{
#if defined(CH5xx)
				USBFSReset();
				jump_isprom();
#else
				// There aren't any other chips that can reboot into USB bootloader, are there?
#endif
			}
		}

/*
		struct SWIOState st;
		int r;
		r = InitializeSWDSWIO( &st );
		if( r == 0 )
		{
			uint8_t reply[8];
			r = DetermineChipTypeAndSectorInfo( &st, reply );
			printf( "Chip type: %d %02x %02x %02x %02x %02x %02x %02x %02x\n",
				r, reply[0], reply[1], reply[2], reply[3], reply[4], reply[5], reply[6], reply[7] );
			uint32_t esig = 0;
			r = ReadWord( &st, 0x1FFFF7E0, &esig );
			printf( "ESIG: %d %08x\n", r, (int)esig );
		}
		Delay_Ms(100);
*/


		if( usb_pending && !USBFSCTX.pCtrlPayloadPtr )
		{
			scratch_return = 0;
			//printf( "+%02x %02x %02x %02x %02x\n", scratch[0], scratch[1], scratch[2], scratch[3], scratch[4] );
			HandleCommandBuffer( scratch );
			//printf( "-%02x %02x %02x %02x %02x\n", scratch[0], scratch[1], scratch[2], scratch[3], scratch[4] );

			__disable_irq();
			if( didnak )
			{
				struct _USBState * ctx = &USBFSCTX;

				int len = didnak;

				if( sizeof(retbuff) - 1 < len )
					len = sizeof(retbuff) - 1;

				ctx->pCtrlPayloadPtr = retbuff;

				ctx->USBFS_SetupReqLen = len;
				len = len >= DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : len;

				uint8_t * ctrl0buff = CTRL0BUFF;
				copyBuffer( ctrl0buff, ctx->pCtrlPayloadPtr, len );
				ctx->pCtrlPayloadPtr += len;

				UEP_CTRL_LEN(0) = len;
				UEP_CTRL_TX(0) = CHECK_USBFS_UEP_T_AUTO_TOG | USBFS_UEP_T_RES_ACK | USBFS_UEP_T_TOG;
				ctx->USBFS_SetupReqLen -= len;

				didnak = 0;
			}
			__enable_irq();

			usb_pending = 0;
			scratch_return = 1;
		}
	}
}



int main()
{
	SystemInit();

	funGpioInitAll();

	R32_PA_PD_DRV = 0;

	funPinMode( LED, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( LED, !LED_ON );

	USBFSReset();

	R32_PA_PU = PIN_SWD;

	printf("USBFS starting...");

	USBFSSetup();

	printf("ok\n");

	funDigitalWrite( LED, LED_ON );

	processLoop();
}

