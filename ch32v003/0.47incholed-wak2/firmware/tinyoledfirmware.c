#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"



// what type of OLED - uncomment just one
//#define SSD1306_64X32
//#define SSD1306_128X32
//#define SSD1306_128X64

#define SSD1306_W 72
#define SSD1306_H 40
#define SSD1306_FULLUSE
#define SSD1306_OFFSET 28

// control pins
#define SSD1306_RST_PIN PC3
#define SSD1306_CS_PIN PC2
#define SSD1306_DC_PIN PC4
#define SSD1306_MOSI_PIN PC6
#define SSD1306_SCK_PIN  PC5

#define SSD1306_BAUD_RATE_PRESCALER SPI_BaudRatePrescaler_2


#include "ssd1306_spi.h"
#include "ssd1306.h"


// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[368];
volatile int start_leds = 0;
static int frame;


int main()
{
	SystemInit();
	funPinMode( PD3, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( PD4, GPIO_CFGLR_OUT_50Mhz_PP );
	funDigitalWrite( PD3, FUN_LOW );
	funDigitalWrite( PD4, FUN_LOW );
	Delay_Ms(1000); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	funGpioInitAll();
	funPinMode( PA1, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( PA2, GPIO_CFGLR_OUT_50Mhz_PP );
	funDigitalWrite( PA1, FUN_LOW );
	funDigitalWrite( PA2, FUN_LOW );

	ssd1306_rst();
	if(ssd1306_spi_init())
	{
		printf( "Could not connect to OLED\n" );
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

	int i;
	/* fill with random */
	for(i=0;i<sizeof(ssd1306_buffer);i++)
	{
		ssd1306_buffer[i] = (i&1)?0xaa:0x55;//(i == (frame&0x1ff) ) ? 0x00: 0xff;
	}
	frame++;

	ssd1306_cmd(SSD1306_COLUMNADDR);
	ssd1306_cmd(28);   // Column start address (0 = reset)
	ssd1306_cmd(28+71); // Column end address (127 = reset)
	
	ssd1306_cmd(SSD1306_PAGEADDR);
	ssd1306_cmd(0); // Page start address (0 = reset)
	ssd1306_cmd(5); // Page end address

	for(i=0;i<sizeof(ssd1306_buffer);i+=SSD1306_PSZ)
	{
		ssd1306_data(&ssd1306_buffer[i], SSD1306_PSZ);
	}

	Delay_Ms(2);

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
			uint8_t * sp = scratch + 4;
	ssd1306_cmd( SSD1306_SETPRECHARGE ); ssd1306_cmd( scratch[1] );
	ssd1306_cmd( SSD1306_SETCONTRAST ); ssd1306_cmd( scratch[2] );
	ssd1306_cmd( SSD1306_SETVCOMDETECT ); ssd1306_cmd( scratch[3] );

		//	printf( "%d\n", start_leds );

			ssd1306_cmd(SSD1306_COLUMNADDR);
			ssd1306_cmd(28);   // Column start address (0 = reset)
			ssd1306_cmd(28+71); // Column end address (127 = reset)
			
			ssd1306_cmd(SSD1306_PAGEADDR);
			ssd1306_cmd(0); // Page start address (0 = reset)
			ssd1306_cmd(5); // Page end address

			ssd1306_data( sp, 180 );
			ssd1306_data( sp+180, 180 );

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


