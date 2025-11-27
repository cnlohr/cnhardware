#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <libusb-1.0/libusb.h>
#include "os_generic.h"

#define USB_VENDOR_ID  0x1234
#define USB_PRODUCT_ID 0x0001

#define USB_TIMEOUT 1024


#define block_size  512

#define TRANSFERS 8


uint8_t buffers[TRANSFERS][block_size];

static libusb_context *ctx = NULL;
static libusb_device_handle *handle;

int abortLoop = 0;

static void sighandler(int signum)
{
	printf( "\nInterrupt signal received\n" );
	abortLoop = 1;
}

int xfertotal = 0;

int Fill( uint8_t * data )
{
	static int ledstate;
	const int leds_per_transfer = 10;
	const int num_leds_per_string = 210;
	static int frame_num;

	uint16_t * lb = (uint16_t*)data;

	// 21 LEDs per bulk transfer.
	int terminal = 0;
	int ledno = ledstate * leds_per_transfer;
	int ledremain = num_leds_per_string - ledno;
	if( ledremain <= leds_per_transfer )
	{
		terminal = 1;
	}
	else
	{
		ledremain = leds_per_transfer;
	}

	lb[0] = (terminal?0x8000:0x0000) | ledstate*leds_per_transfer*24; // Offset
	lb[1] = ledremain * 24;

	uint16_t * lbo = lb+2;

	int l;
	for( l = 0; l < leds_per_transfer; l++ )
	{
		int bit;
		int ledid = leds_per_transfer * ledstate + l;
		for( bit = 0; bit < 24; bit++ )
		{
			if( ( frame_num % num_leds_per_string ) != ledid )
				lbo[bit+l*24] = 0x0000;
			else
				lbo[bit+l*24] = 0xffff;
		}
	}


	if( terminal )
	{
		frame_num++;
		ledstate = 0;
	}
	else
	{
		ledstate++;
	}

	return 512;
}

void xcallback (struct libusb_transfer *transfer)
{
	xfertotal += transfer->actual_length;
	transfer->length = Fill( transfer->buffer );
	libusb_submit_transfer( transfer );
}


int main(int argc, char **argv)
{
	//Pass Interrupt Signal to our handler
	signal(SIGINT, sighandler);

	libusb_init(&ctx);
	libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 3);

	handle = libusb_open_device_with_vid_pid( ctx, USB_VENDOR_ID, USB_PRODUCT_ID );
	if ( !handle )
	{
		fprintf( stderr, "Error: couldn't find handle\n" );
		return 1;
	}

	libusb_detach_kernel_driver(handle, 3);

	int r = 1;
	r = libusb_claim_interface(handle, 3 );
	if( r < 0 )
	{
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		return 2;
	}

	double dRecvTotalTime = 0;
	double dSendTotalTime = 0;
	double dLastPrint = OGGetAbsoluteTime();
	int rtotal = 0, stotal = 0;

	// Async (Fast, bulk)
	// About 260-320 Mbit/s

	struct libusb_transfer * transfers[TRANSFERS];
	int n;
	for( n = 0; n < TRANSFERS; n++ )
	{
		int k;
		for( k = 0; k < block_size; k++ )
		{
			buffers[n][k] = k;
		}
		struct libusb_transfer * t = transfers[n] = libusb_alloc_transfer( 0 );
		libusb_fill_bulk_transfer( t, handle, 0x05 /*Endpoint for send */, buffers[n], block_size, xcallback, (void*)(intptr_t)n, 1000 );
		t->length = Fill( t->buffer );
		libusb_submit_transfer( t );
	}


	while(!abortLoop)
	{
			double dNow = OGGetAbsoluteTime();

			libusb_handle_events(ctx);

			if( dNow - dLastPrint > 1 )
			{
				dSendTotalTime = dNow - dLastPrint;
				printf( "%f MB/s %cX\n", xfertotal / (dSendTotalTime * 1024 * 1024), 'T' );
				xfertotal = 0;
				dSendTotalTime = 0;
				dLastPrint = dNow;
			}
	}

	for( n = 0; n < TRANSFERS; n++ )
	{
		libusb_cancel_transfer( transfers[n] );
	 	libusb_free_transfer( transfers[n] );
	}

	libusb_release_interface (handle, 0);
	libusb_close(handle);
	libusb_exit(NULL);

	return 0;
}

