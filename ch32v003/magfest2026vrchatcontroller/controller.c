#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

#define WS2812DMA_IMPLEMENTATION
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define NR_LEDS 191

#include "ws2812b_dma_spi_led_driver.h"

int led_hit;

// Callbacks that you must implement.
uint32_t WS2812BLEDCallback( int ledno )
{
	return (ledno == led_hit) ? 0xff00ff : 0x00; // Where "tween" is a value from 0 ... 255
}

typedef struct {
	volatile uint8_t len;
	uint8_t buffer[8];
} midi_message_t;

midi_message_t midi_in;

// midi data from device -> host (IN)
void midi_send(uint8_t * msg, uint8_t len)
{
	memcpy( midi_in.buffer, msg, len );
	midi_in.len = len;
}
#define midi_send_ready() (midi_in.len == 0)

// Set up PC2 to PC5 as inputs
void keyboard_init( void )
{
	// Enable GPIOC
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOA;

	funDigitalWrite( PD2, 1 );
	funDigitalWrite( PD6, 1 );
	funDigitalWrite( PD7, 1 );
	funDigitalWrite( PC0, 1 );
	funDigitalWrite( PC1, 1 );
	funDigitalWrite( PC2, 1 );
	funDigitalWrite( PC3, 1 );
	funDigitalWrite( PC4, 1 );
	funDigitalWrite( PC5, 1 );
	funDigitalWrite( PC7, 1 );
	funPinMode( PD2, GPIO_CFGLR_IN_PUPD );
	funPinMode( PD6, GPIO_CFGLR_IN_PUPD );
	funPinMode( PD7, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC0, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC1, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC2, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC3, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC4, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC5, GPIO_CFGLR_IN_PUPD );
	//funPinMode( PC6, GPIO_CFGLR_IN_PUPD ); // MOSI (will be WS2812?)
	funPinMode( PC7, GPIO_CFGLR_IN_PUPD );
}

void scan_keyboard(void)
{
	static uint16_t clickcount = 0;
	uint32_t input = GPIOC->INDR | ( GPIOD->INDR << 8 );
	static uint32_t last_input;
	input &= 0b1100010010111111;
	
	int dcc = 0;
	int i;
	uint32_t mask = input ^ last_input;
	
	mask &= ~input;
	for( i = 0; i < 16; i++ )
	{
		if( mask & (1<<i) )
		{
			dcc++;
		}
	}
	clickcount+=dcc;
	//printf( "%04x %d\n", mask, clickcount );

	if( midi_send_ready() )
	{
		uint8_t midi_msg[4] = {0x09, 0xB0, 0x40, 0x7F}; // Control Channel
		midi_msg[2] = (clickcount >> 3);
		midi_msg[3] = (clickcount & 7)<<4;
		midi_send(midi_msg, 4);

		led_hit = clickcount%6;
		
		WS2812BDMAStart( 192 );
	}

	last_input = input;
}

int main()
{
	WS2812BDMAInit( );

	SystemInit();
	keyboard_init();
	usb_setup();
	while(1)
	{
		scan_keyboard();
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if (endp && midi_in.len) {
		usb_send_data( midi_in.buffer, midi_in.len, 0, sendtok );
		midi_in.len = 0;
	} else {
		usb_send_empty( sendtok );
	}
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
//	if (len); //midi_receive(data);
//	if (len==8); //midi_receive(&data[4]);
}






/*

using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

using TMPro;
public class MidiReceiver : UdonSharpBehaviour
{
	public TMP_Text tmp;
	public int frameno;
	
	public int lastVelocity;
	public int lastNoteNo;
	public int lastPercent420;
	
    void Start()
    {
        Debug.Log( "test" );
    }
	
	override public void MidiControlChange(int channel, int number, int velocity)
	{
		Debug.Log( $"MidiControlChange {channel} {number} {velocity}" );
		lastVelocity = velocity;
		lastNoteNo = number;
		lastPercent420 = number * 8 + velocity/16;
	}
	
	override public void MidiNoteOn(int channel, int number, int velocity)
	{
		Debug.Log( $"MidiNoteOn {channel} {number} {velocity}" );
		lastVelocity = velocity;
		lastNoteNo = number;
		lastPercent420 = number * 8 + velocity/16;
	}
	
	
	void Update()
	{
		frameno++;
		tmp.text = $"N:{frameno}\n{lastPercent420}";
	}
}
*/