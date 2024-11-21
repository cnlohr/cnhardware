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
	if( mode == 2 )
	{
		funPinMode( pin, GPIO_CFGLR_IN_FLOAT );
	}
	else
	{
		funDigitalWrite( pin, mode );
		funPinMode( pin, GPIO_CFGLR_IN_PUPD );
	}
}

void UpdateLCD( uint32_t mask )
{
	int group;


	int invmask = (~mask);

	uint8_t pinset[4] = { LCDCOM1, LCDCOM2, LCDCOM3, LCDCOM4 };

	uint32_t  tmask = mask;
	uint32_t itmask = invmask;
	LCDSEGBUF->BSHR = 0x3f;

	for( group = 0; group < 4; group++ )
		ConfigPinTri( pinset[group], 2 );
	LCDSEGBUF->BSHR = 0x3f<<16;

	for( group = 0; group < 4; group++ )
	{
		LCDSEGBUF->BSHR = tmask&0x3f;
		ConfigPinTri( pinset[group], 0 );
		Delay_Us(800);
		LCDSEGBUF->BSHR = 0x3f<<16;
		ConfigPinTri( pinset[group], 1 );
		Delay_Us(2);
		ConfigPinTri( pinset[group], 2 );
		tmask>>=6;
	}

	LCDSEGBUF->BSHR = 0x3f<<16; // Setting this to ON makes things fade-y-er
	ConfigPinTri( pinset[0], 1 );
	ConfigPinTri( pinset[1], 1 );
	ConfigPinTri( pinset[2], 1 );
	ConfigPinTri( pinset[3], 1 );
	//Delay_Us(2); // This forces it sharper, but at the cost of everything being darker.
	LCDSEGBUF->BSHR = 0x3f;

	Delay_Us(1000);

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
		UpdateLCD( id );
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


