// Small example showing how to use the USB HS interface of the CH32V30x
// A composite HID device + A bulk in and out.

#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "hsusb_v30x.h"
#include "hsusb_v30x.c" // Normally would be in ADDITIONAL_C_FILES, but the PIO build system doesn't currently understand that.

uint32_t count;
int dummy = 1; // Dummy data being sent (no host)
int streaming = 0;
int need_to_confirm = 0;

#define BUFFER_SIZE_SHORTS 6120
volatile uint16_t memory_buffer[BUFFER_SIZE_SHORTS] __attribute__((aligned(4)));


int last = 0;
void handle_debug_input( int numbytes, uint8_t * data )
{
	last = data[0];
	count += numbytes;
}

int lrx = 0;
uint8_t scratchpad[2048];

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
	if( id == 0xaa )
	{
		ctx->pCtrlPayloadPtr = scratchpad;
		if( sizeof(scratchpad) - 1 < lrx )
			return sizeof(scratchpad) - 1;
		else
			return lrx;
	}
	return 0;
}

void HandleHidUserReportDataOut( struct _USBState * ctx, uint8_t * data, int len )
{
}

int HandleHidUserReportDataIn( struct _USBState * ctx, uint8_t * data, int len )
{
	return len;
}

void HandleHidUserReportOutComplete( struct _USBState * ctx )
{
	return;
}


void HandleGotEPComplete( struct _USBState * ctx, int ep )
{
	if( ep == 5 )
	{
		dummy = 0;

		if( streaming )
		{
			need_to_confirm = 1;
			return;
		}

		need_to_confirm = 0;

		// Received data is written into scratchpad,
		// and USBHSD->RX_LEN

		uint16_t * sp = (uint16_t*)scratchpad;
		int term = sp[0] >> 15;
		int offset = sp[0] & 0x7fff; // in words
		int remain = sp[1]; // In words

		int ofs_start = offset;
		int ofs_end = offset + remain + 1;

		if( ofs_start > sizeof(memory_buffer)/sizeof(memory_buffer[0]) )
		{
			ofs_start = sizeof(memory_buffer)/sizeof(memory_buffer[0]);
		}

		if( ofs_end > sizeof(memory_buffer)/sizeof(memory_buffer[0]) )
		{
			ofs_end = sizeof(memory_buffer)/sizeof(memory_buffer[0]);
		}

		// If invalid just truncate.
		if( ofs_end - ofs_start > 510 )
			ofs_end = ofs_start + 510;

		int o = ofs_start;
		uint16_t * spp = sp + 2;
		//printf( "%d %d %d %08x %d %d %d %d\n", USBHSD->RX_LEN, sp[0], sp[1], spp, o, ofs_end, term, remain );
		//printf( "TRUNC: %d %d\n", offset, ofs_end - ofs_start );
		//printf( "." );

		while( o != ofs_end )
		{
			memory_buffer[o] = *(spp++);
			o++;
		}

		if( term )
		{
			//printf( "OE: %d\n", ofs_end );
			DMA1_Channel3->CNTR = ofs_end;// sizeof(memory_buffer) / sizeof(memory_buffer[0]);
			DMA1_Channel3->MADDR = (uint32_t)memory_buffer;
			DMA1_Channel3->PADDR = (uint32_t)&GPIOC->OUTDR;
			DMA1_Channel3->CFGR = 
				DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
				DMA_CFGR1_PL |                       // High priority.
				DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
				DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
				DMA_CFGR1_MINC |                     // Increase memory.
				0 |                                  // Circular mode.
				DMA_CFGR1_HTIE |                     // Half-trigger
				DMA_CFGR1_TCIE |                     // Whole-trigger
				DMA_CFGR1_EN;                        // Enable

			TIM1->CTLR1 = TIM_CEN;
			streaming = 1;
		}
		else
		{
			HSUSBCTX.USBHS_Endp_Busy[5] = 0;
		}

		// You must re-up these, not sure why but they get corrupt.
		USBHSD->UEP5_RX_DMA = (uintptr_t)scratchpad;
		USBHSD->UEP5_MAX_LEN = 1024;
		USBHSD->UEP5_RX_CTRL ^= USBHS_UEP_R_TOG_DATA1;
		USBHSD->UEP5_RX_CTRL = ((USBHSD->UEP5_RX_CTRL) & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
	}
}

int main()
{
	SystemInit();

	funGpioInitAll();


	// No idea why, these need to be here.
	volatile uint16_t set_zero[1] __attribute__((aligned(4))) =  { 0x0000 };
	volatile uint16_t set_one[1]  __attribute__((aligned(4))) =  { 0xffff };


	funPinMode( PA3, GPIO_CFGLR_OUT_10Mhz_PP );

    Delay_Ms(200);
	// Enable DMA
	HSUSBSetup();

	RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_DMA2 | RCC_AHBPeriph_USBHS;
	RCC->APB2PCENR = RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO  | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE;

	funPinMode( PA9, GPIO_CFGLR_OUT_50Mhz_PP );
	funDigitalWrite( PA9, 0);


	GPIOC->CFGLR = 0x11111111; // All output.  All 10MHz
	GPIOC->CFGHR = 0x11111111; // All output.  All 10MHz

	set_one[0] = 0xffff;

	// DMA2 = T1CH1
	DMA1_Channel2->CNTR = 1;
	DMA1_Channel2->MADDR = (uint32_t)set_one;
	DMA1_Channel2->PADDR = (uint32_t)&GPIOC->OUTDR;
	DMA1_Channel2->CFGR = 
		DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
		DMA_CFGR1_PL |                       // High priority.
		DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
		DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
		0 |                     // Do not increase memory
		DMA_CFGR1_CIRC |                     // Circular mode.
		0 |                     // Half-trigger
		0 |                     // Whole-trigger
		DMA_CFGR1_EN;                        // Enable

    // DMA3 = T1C2
	DMA1_Channel3->CNTR = sizeof(memory_buffer) / sizeof(memory_buffer[0]);
	DMA1_Channel3->MADDR = (uint32_t)memory_buffer;
	DMA1_Channel3->PADDR = (uint32_t)&GPIOC->OUTDR;
	DMA1_Channel3->CFGR = 
		DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
		DMA_CFGR1_PL |                       // High priority.
		DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
		DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		0 |                     // Circular mode.
		DMA_CFGR1_HTIE |                     // Half-trigger
		DMA_CFGR1_TCIE |                     // Whole-trigger
		DMA_CFGR1_EN;                        // Enable


    // DMA3 = T1C3
	DMA1_Channel6->CNTR = 1;
	DMA1_Channel6->MADDR = (uint32_t)set_zero;
	DMA1_Channel6->PADDR = (uint32_t)&GPIOC->OUTDR;
	DMA1_Channel6->CFGR = 
		DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
		DMA_CFGR1_PL |                       // High priority.
		DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
		DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
		0 |                     // Do not increase memory
		DMA_CFGR1_CIRC |                     // Circular mode.
		0 |                     // Half-trigger
		0 |                     // Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	// Setup Timer1.
	RCC->APB2PRSTR = RCC_APB2Periph_TIM1;    // Reset Timer
	RCC->APB2PRSTR = 0;

	// Timer 1 setup.
	// Timer 1 is what will trigger the DMA, Channel 2 engine.
	TIM1->PSC = 0x0003;                      // Prescaler 
	TIM1->ATRLR = 33;                        // Minimum = 32 --> Selecting 33
	TIM1->SWEVGR = TIM_UG | TIM_TG;          // Reload immediately + Trigger DMA

	TIM1->CH1CVR = 0;                        // T1C1 triggers DMA
	TIM1->CH2CVR = 12;                        // T1C1 triggers DMA // Minimum = 9
	TIM1->CH3CVR = 24;                       // T1C1 triggers DMA // Minimum = 22
	TIM1->CTLR1 = TIM_CEN;                   // Enable TIM1
	TIM1->DMAINTENR = TIM_CC1DE | TIM_CC2DE | TIM_CC3DE;   // Trigger DMA on TC match 1 (DMA Ch2) and TC match 2 (DMA Ch3)


	// Override EP5
	USBHSD->UEP5_MAX_LEN = 1024; // Max allowed.
	USBHSD->UEP5_RX_DMA = (uintptr_t)scratchpad;

	int tickcount = 0;
    int loopcount = 0;



	while(1)
	{
		//printf( "%lu %08lx %lu %d %d\n", USBDEBUG0, USBDEBUG1, USBDEBUG2, 0, 0 );

/*
		// Send data back to PC.
		if( !( HSUSBCTX.USBHS_Endp_Busy[4] & 1 ) )
		{
			USBHS_SendEndpoint( 4, 512, scratchpad );
		}


		int i;
		for( i = 1; i < 3; i++ )
		{

			uint32_t * buffer = (uint32_t*)USBHS_GetEPBufferIfAvailable( i );
			if( buffer )
			{
				int tickDown = ((SysTick->CNT)&0x800000);
				static int wasTickMouse, wasTickKeyboard;
				if( i == 1 )
				{
					// Keyboard  (only press "8" 3 times)
					if( tickcount <= 3 && tickDown && !wasTickKeyboard )
					{
						buffer[0] = 0x00250000;
						tickcount++;
					}
					else
					{
						buffer[0] = 0x00000000;
					}
					buffer[1] = 0x00000000;
					wasTickKeyboard = tickDown;
				}
				else
				{
					buffer[0] = (tickDown && !wasTickMouse)?0x0010100:0x00000000;
					wasTickMouse = tickDown;
				}
				USBHS_SendEndpoint( i, (i==1)?8:4, 0 );
			}
		}
*/

		__disable_irq();
		// Critical section.  Todo: Fixme Kinda jank.
		if( DMA1_Channel3->CNTR == 0 && streaming)
		{
			TIM1->CTLR1 = 0;
			GPIOC->OUTDR = 0;
			TIM1->CNT = 0;
			DMA1_Channel3->CFGR = 0;
			streaming = 0;
			if( need_to_confirm )
			{
				HandleGotEPComplete( &HSUSBCTX, 5 );
			}
		}
		__enable_irq();


		if( dummy )
		{
	        loopcount++;

			if( DMA1_Channel3->CNTR == 0 )
			{
				TIM1->CTLR1 = 0;
				GPIOC->OUTDR = 0;
				TIM1->CNT = 0;
				DMA1_Channel3->CFGR = 0;
				streaming = 0;

				Delay_Us(280);
				static int frame;
				frame++;

				int i;

				for( i = 0; i < BUFFER_SIZE_SHORTS; i++ )
				{
				   // memory_buffer[i] = (i&1)?0xffff:0x0000;
					int lno = i/24;
					int pos = i-(lno*24);

					if( ( lno + (frame>>5) ) & 1 )
					{
						if( pos < 8 )
							memory_buffer[i] = 0xffff;
						else if( pos < 16 )
							memory_buffer[i] = 0x0000;
						else
							memory_buffer[i] = 0xffff;
					}
					else
					{
						if( pos < 8 )
							memory_buffer[i] = 0x0000;
						else if( pos < 16 )
							memory_buffer[i] = 0x0000;
						else
							memory_buffer[i] = 0xffff;
					}
				}


				DMA1_Channel3->CNTR = sizeof(memory_buffer) / sizeof(memory_buffer[0]);
				DMA1_Channel3->MADDR = (uint32_t)memory_buffer;
				DMA1_Channel3->PADDR = (uint32_t)&GPIOC->OUTDR;
				DMA1_Channel3->CFGR = 
					DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
					DMA_CFGR1_PL |                       // High priority.
					DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
					DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
					DMA_CFGR1_MINC |                     // Increase memory.
					0 |                     // Circular mode.
					DMA_CFGR1_HTIE |                     // Half-trigger
					DMA_CFGR1_TCIE |                     // Whole-trigger
					DMA_CFGR1_EN;                        // Enable

				TIM1->CTLR1 = TIM_CEN;
				streaming = 1;
			}
		}

	}
}

