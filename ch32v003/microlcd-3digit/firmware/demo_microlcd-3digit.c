#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];
volatile uint8_t start_leds = 0;
uint8_t ledat;

#define LCDCOM1 PD0
#define LCDCOM2 PD7
#define LCDCOM3 PD6
#define LCDCOM4 PD2

#define LCDSEGBUF GPIOC

#define LEDSEG0 PC0
#define LEDSEG1 PC1
#define LEDSEG2 PC2
#define LEDSEG3 PC3
#define LEDSEG4 PC4
#define LEDSEG5 PC5


#define drivepr GPIO_CFGLR_IN_PUPD
//#define drivepr GPIO_CFGLR_OUT_2Mhz_PP
#define drivenr GPIO_CFGLR_IN_FLOAT


static uint8_t frame;

// LCD Driving from https://www.nxp.com/docs/en/application-note/AN3219.pdf

static void ConfigPinTri( int pin, int mode )
{
	funDigitalWrite( pin, mode );
}

void UpdateLCD( uint32_t mask )
{
	int group;

	// Digit 0
	// 0,1,6,7,12,13,18,19
	//mask = (0<<18) | (1<<12);
	int onemask = mask & 0x7f;
	//printf( "%02x\n", onemask );

//	mask = (onemask&1) | (onemask&2) | (onemask&4)<<(6-2) | (onemask&8)<<(7-3) | (onemask&16)<<(12-4) |
//			(onemask&32)<<(13-5) | (onemask&64)<<(18-6) | (1<<19); //((onemask&128)<<(19-7));

	mask = ((onemask&1)<<0) | (1<<13);

	int invmask = (~mask);

	const uint8_t patterntable[] = {
		1, 0, 0, 0, // Odd 0
		0, 1, 0, 0, // Odd 1
		0, 0, 1, 0, // Odd 2
		0, 0, 0, 1, // Odd 3
		0, 0, 0, 0, // C (Odd)
		0, 1, 1, 1, // Even 0
		1, 0, 1, 1, // Even 1
		1, 1, 0, 1, // Even 2
		1, 1, 1, 0, // Even 3
		1, 1, 1, 1, // C (Even)
	};
	const uint8_t * ptable = patterntable;

	uint32_t  tmask = mask;
	uint32_t itmask = invmask;

	uint8_t pinset[4] = { LCDCOM1, LCDCOM2, LCDCOM3, LCDCOM4 };
	ConfigPinTri( pinset[0], 0 );
	ConfigPinTri( pinset[1], 0 );
	ConfigPinTri( pinset[2], 0 );
	ConfigPinTri( pinset[3], 0 );

	for( group = 0; group < 4; group++ )
	{
		int lmask = tmask & 0x3f;
		int imask = itmask & 0x3f;

		ConfigPinTri( pinset[group], 1 );
		LCDSEGBUF->BSHR = lmask | (imask << 16);

		ptable += 4;
		tmask >>= 6;
		itmask >>= 6;
		Delay_Us(4000);
		ConfigPinTri( pinset[group], 0 );
	}

	LCDSEGBUF->BSHR = (0x3f << 16);


	ptable += 4;
	Delay_Us(1000);

	ConfigPinTri( pinset[0], 1 );
	ConfigPinTri( pinset[1], 1 );
	ConfigPinTri( pinset[2], 1 );
	ConfigPinTri( pinset[3], 1 );

#if 1
	 tmask = mask;
	itmask = invmask;
	for( group = 0; group < 4; group++ )
	{
		int lmask = tmask & 0x3f;
		int imask = itmask & 0x3f;

		ConfigPinTri( pinset[group], 0 );
		LCDSEGBUF->BSHR = imask | (lmask << 16);
		ptable += 4;
		tmask >>= 6;
		itmask >>= 6;
		Delay_Us(4000);
		ConfigPinTri( pinset[group], 1 );
	}
#endif
	LCDSEGBUF->BSHR = (0x3f);
	ptable += 4;
	Delay_Ms(10);
}


int main()
{
	SystemInit();
	Delay_Ms(3); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	funGpioInitAll();

	funPinMode( LCDCOM1, drivepr );
	funPinMode( LCDCOM2, drivepr );
	funPinMode( LCDCOM3, drivepr );
	funPinMode( LCDCOM4, drivepr );
	funPinMode( LEDSEG0, drivepr );
	funPinMode( LEDSEG1, drivepr );
	funPinMode( LEDSEG2, drivepr );
	funPinMode( LEDSEG3, drivepr );
	funPinMode( LEDSEG4, drivepr );
	funPinMode( LEDSEG5, drivepr );

	int id = 0;

	while(1)
	{
		id++;
		UpdateLCD( id >> 3 );
	}
	while(1)
	{
		//printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
		if( start_leds )
		{
			//WS2812BSimpleSend( GPIOC, 6, scratch + 3, 6*3 );
			//ledat = 3;
			//if( scratch[1] == 0xa4 )
			//{
			//	WS2812BDMAStart( 254/4 );
			//}
			start_leds = 0;
			frame++;
		}
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	// Make sure we only deal with control messages.  Like get/set feature reports.
	if( endp )
	{
		usb_send_empty( sendtok );
	}
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	//LogUEvent( SysTick->CNT, current_endpoint, e->count, 0xaaaaaaaa );
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			start_leds = e->max_len;
		}
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );

	// You can check the lValueLSBIndexMSB word to decide what you want to do here
	// But, whatever you point this at will be returned back to the host PC where
	// it calls hid_get_feature_report. 
	//
	// Please note, that on some systems, for this to work, your return length must
	// match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h

	e->opaque = scratch;
	e->max_len = reqLen;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	// Here is where you get an alert when the host PC calls hid_send_feature_report.
	//
	// You can handle the appropriate message here.  Please note that in this
	// example, the data is chunked into groups-of-8-bytes.
	//
	// Note that you may need to make this match HID_REPORT_COUNT, in your HID
	// report, in usb_config.h

	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
}


void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}


