// Modded to take input on PD6

#include "ch32fun.h"
#include <stdio.h>
#include <string.h>

#define WS2812DMA_IMPLEMENTATION
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define NR_LEDS 252

#include "ws2812b_dma_spi_led_driver.h"

#include "color_utilities.h"

#include "rainbowphase.h"

uint16_t phases[NR_LEDS];
int frameno;
volatile int tween = -NR_LEDS;

#define BRIGHTNESS_QUEUE 128 // 
uint8_t brightnessqueue[BRIGHTNESS_QUEUE];
uint8_t active_mode[BRIGHTNESS_QUEUE];


// Callbacks that you must implement.
uint32_t WS2812BLEDCallback( int ledno )
{
#if 1
	int selo = brightnessqueue[(ledno>>2)+63];
	int tfo = active_mode[(ledno>>2)+63];
	if( (tfo << 2) > selo )
	{
		uint8_t index = (phases[ledno])>>8;
		uint8_t rsbase = sintable[index];
		rsbase = (((uint16_t)rsbase) * tfo)>>8;

		return (tfo<<16) | ((rsbase)<<8) | ((rsbase));
	}
	else
	{
		uint8_t rp = rainbowphase[ledno] + frameno;

		int r = huetable[(rp)&0xff];
		int g = huetable[(rp+83)&0xff];
		int b = huetable[(rp+171)&0xff];

		r = (r * selo)>>8;
		g = (g * selo)>>8;
		b = (b * selo)>>8;

		return r | (g<<8) | (b<<16);
		
	}
#endif
#if 0
	uint8_t rp = rainbowphase[ledno] + frameno;

	int r = huetable[(rp)&0xff];
	int g = huetable[(rp+83)&0xff];
	int b = huetable[(rp+171)&0xff];

	return r | (g<<8) | (b<<16);

#endif

//	return TweenHexColors( fire, ice, ((tween + ledno)>0)?255:0 ); // Where "tween" is a value from 0 ... 255
}

int main()
{
	int k;
	SystemInit();

	// Enable GPIOD (for debugging)
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

	funPinMode( PD0, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

	GPIOD->BSHR = 1;	 // Turn on GPIOD0

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOA | RCC->APB2PCENR | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO;

	// Enable USART
	// Set UART baud rate here
	#define UART_BR 115200

	funPinMode( PD6, GPIO_Speed_In | GPIO_CNF_IN_PUPD );
	funDigitalWrite( PD6, 1 );

	AFIO->PCFR1 = 0; // No remap, RX on PD6

	// Setup UART for Rx 8n1
	USART1->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Rx;
	USART1->CTLR2 = USART_StopBits_1;

	// Set baud rate and enable UART
	USART1->BRR = ((FUNCONF_SYSTEM_CORE_CLOCK) + (UART_BR)/2) / (UART_BR);
	USART1->CTLR1 |= CTLR1_UE_Set;

	frameno = 0;

	for( k = 0; k < NR_LEDS; k++ ) phases[k] = k<<8;

	int tweendir = 0;

	int last_num = 0;


	WS2812BDMAInit( );



	while(1)
	{
		uint8_t ue = USART1->DATAR;
		int i;
//printf( "%02x\n", ue );
		for( i = BRIGHTNESS_QUEUE - 1; i > 0; i--)
		{
			brightnessqueue[i] = brightnessqueue[i-1];
			active_mode[i] = active_mode[i-1];
		}

		int delta = (ue&0x3f) - last_num;
		if( delta < 0 ) delta += 0x40;

		if( ue & 0x80 )
		{
			// Full!
			active_mode[0] = 255;
		}
		else
		{
			if( active_mode[0] > 0 ) active_mode[0]--;
			if( delta )
			{
				brightnessqueue[0] = 255;
			}
			else
			{
				int bql = brightnessqueue[0];
				bql = bql - (bql>>2) - 1;
				if( bql < 0 ) bql = 0;
				brightnessqueue[0] = bql;
			}
		}
		last_num += delta;

		GPIOD->BSHR = 1;	 // Turn on GPIOD0
		// Wait for LEDs to totally finish.
		Delay_Ms( 12 );
		GPIOD->BSHR = 1<<16; // Turn it off

		while( WS2812BLEDInUse );

		frameno++;

		if( frameno == 1024 )
		{
			tweendir = 1;
		}
		if( frameno == 2048 )
		{
			tweendir = -1;
			frameno = 0;
		}

		if( tweendir )
		{
			int t = tween + tweendir;
			if( t > 255 ) t = 255;
			if( t < -NR_LEDS ) t = -NR_LEDS;
			tween = t;
		}

		for( k = 0; k < NR_LEDS; k++ )
		{
			phases[k] += ((((rands[k&0xff])+0xf)<<2) + (((rands[k&0xff])+0xf)<<1))>>1;
		}

		WS2812BDMAStart( NR_LEDS );
	}
}

