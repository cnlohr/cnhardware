#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

uint32_t clickcount = 0;
uint32_t clickcount_raw = 0;
uint32_t activate_time = 0;
int is_activated = 0;

//6 seconds
#define BONUS_TIME 288000000
#define MAX_CLICKS 420

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
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOA | RCC->APB2PCENR | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO;

	funDigitalWrite( PA1, 1 );
	funDigitalWrite( PA2, 1 );
	funDigitalWrite( PD2, 1 );
	funDigitalWrite( PD6, 1 );
	funDigitalWrite( PD7, 1 );
	funDigitalWrite( PC0, 1 );
	funDigitalWrite( PC1, 1 );
	funDigitalWrite( PC2, 1 );
	funDigitalWrite( PC3, 1 );
	funDigitalWrite( PC4, 1 );
	funDigitalWrite( PC5, 1 );
	funDigitalWrite( PC6, 1 );
	funDigitalWrite( PC7, 1 );

	funPinMode( PA1, GPIO_CFGLR_IN_PUPD );
	funPinMode( PA2, GPIO_CFGLR_IN_PUPD );
	funPinMode( PD2, GPIO_CFGLR_IN_PUPD );
	funPinMode( PD6, GPIO_CFGLR_IN_PUPD );
	funPinMode( PD7, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC0, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC1, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC2, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC3, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC4, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC5, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC6, GPIO_CFGLR_IN_PUPD );
	funPinMode( PC7, GPIO_CFGLR_IN_PUPD );


	// Set UART baud rate here
	#define UART_BR 115200

	// Push-Pull, 10MHz Output on D0, with AutoFunction, USART1_RM=01
	funPinMode( PD0, GPIO_CFGLR_OUT_10Mhz_AF_PP );

	AFIO->PCFR1 = AFIO_PCFR1_USART1_REMAP;

	// Setup UART for Tx 8n1
	USART1->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Tx;
	USART1->CTLR2 = USART_StopBits_2;

	// Set baud rate and enable UART
	USART1->BRR = ((FUNCONF_SYSTEM_CORE_CLOCK) + (UART_BR)/2) / (UART_BR);
	USART1->CTLR1 |= CTLR1_UE_Set;
}

void scan_keyboard(void)
{
	uint32_t input = GPIOC->INDR | ( GPIOD->INDR << 8 ) | ( GPIOA->INDR<<16);
	static uint32_t last_input;
	static uint32_t last_down[24];
	input &= 0b1101100010011111111;
	
	int dcc = 0;
	int i;
	uint32_t mask = input ^ last_input;
	mask &= ~input;
	uint32_t now = SysTick->CNT;
	static uint32_t pending = 0;

	// Debounce down press only.
	for( i = 0; i < 24; i++ )
	{
		if( (mask & (1<<i)) )
		{
			last_down[i] = now;
			pending |= (1<<i);
		}
		if( (now - last_down[i]) > 960000 ) //20ms debounce
		{
			if( (pending&(1<<i)) )
			{
				if( ( input & (1<<i)) == 0 ) 
					dcc++;
			}
			pending&=~(1<<i);
		}

	}
	clickcount+=dcc;
	clickcount_raw+=dcc;

	if( clickcount >= MAX_CLICKS && !is_activated )
	{
		clickcount = MAX_CLICKS;
		is_activated = 1;
		activate_time = SysTick->CNT;
	}

	if( is_activated && ( (SysTick->CNT - activate_time) > BONUS_TIME ) )
	{
		is_activated = 0;
		activate_time = 0;
		clickcount = 0;
	}

	if( midi_send_ready() )
	{
		uint8_t midi_msg[4] = {0x09, 0xB0, 0x40, 0x7F}; // Control Channel
		midi_msg[2] = (clickcount >> 3);
		midi_msg[3] = (clickcount & 7)<<4;
//printf( "%d\n", clickcount );
		midi_send(midi_msg, 4);
	}

	last_input = input;
}

int main()
{
	SystemInit();
	keyboard_init();
	usb_setup();
	int fct = 0;
	while(1)
	{
		scan_keyboard();

		if((fct & 0x7)== 0)
		{
			USART1->DATAR = (clickcount_raw & 0x3f) | ( is_activated ? 0x80 : 0x00);
			//printf( "!\n" );
		}
		Delay_Us(200);
		fct++;
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
