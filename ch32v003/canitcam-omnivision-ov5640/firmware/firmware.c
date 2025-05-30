#include "ch32fun.h"
#include <stdio.h>

// Connections
//  PA1 = XCLK <<<< Test
//  PD0 = SCL
//  PD3 = SDA
//  PD4 = PCLK <<<< Test
//  PD2 = VSYNC

#define PIN_XCK   PD6
#define PIN_SCL   PD0
#define PIN_SDA   PD3
#define PIN_PCK   PD4
#define PIN_VS    PD2 // NOT USED
#define PIN_HS    PD5 // NEEDED
#define PIN_RST   PD7

#define DELAY1 Delay_Us(1);
#define DELAY2 Delay_Us(2);
#define DSCL_IHIGH  { funPinMode( PIN_SCL, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SCL, 1 ); } 
#define DSDA_IHIGH  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSDA_INPUT  { funPinMode( PIN_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( PIN_SDA, 1 ); } 
#define DSCL_OUTPUT { funDigitalWrite( PIN_SCL, 0 ); funPinMode( PIN_SCL, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define DSDA_OUTPUT { funDigitalWrite( PIN_SDA, 0 ); funPinMode( PIN_SDA, GPIO_CFGLR_OUT_2Mhz_PP );  } 
#define READ_DSDA    funDigitalRead( PIN_SDA )

#define I2CNEEDGETBYTE 1
#define I2CNEEDSCAN    1

#define CAMERA_ADDRESS 0x78

#include "static_i2c.h"

void ConfigureCamera();
int CamReadReg( unsigned int addy );
int CamWriteReg( unsigned int addy, unsigned int data );


uint8_t rawBuffer[1836] __attribute__((aligned(8)));

int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();

	// Enable DMA
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;

	// Enable timers and remapping.
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	funPinMode( PIN_VS,    GPIO_CFGLR_IN_FLOAT );
	funPinMode( PIN_HS,    GPIO_CFGLR_IN_FLOAT );
	funPinMode( PIN_RST,   GPIO_CFGLR_OUT_2Mhz_PP );

	// Port C Configured as pure input.
	GPIOC->CFGLR = 0x44444444; // GPIO_CFGLR_IN_FLOAT = 4
	

	funDigitalWrite( PIN_RST, 0 );

	ConfigI2C();



	// DMA2 can be configured to attach to T1CH1
	// The system can only DMA out at ~2.2MSPS.  2MHz is stable.
	// The idea here is that this copies, byte-at-a-time from the memory
	// into the peripheral addres.
	DMA1_Channel4->CNTR = sizeof(rawBuffer) / sizeof(rawBuffer[0]);
	DMA1_Channel4->MADDR = (uint32_t)rawBuffer;
	DMA1_Channel4->PADDR = (uint32_t)&GPIOC->INDR; // This is the output register for out buffer.
	DMA1_Channel4->CFGR = 
		DMA_DIR_PeripheralSRC |              // !MEM2PERIPHERAL
		DMA_CFGR1_PL_1 |                       // High priority.
		0 |                                  // 8-bit memory
		0 |                                  // 8-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		DMA_CFGR1_CIRC |                     // Circular mode.
		/*DMA_CFGR1_HTIE |*/                    // Half-trigger
		/*DMA_CFGR1_TCIE |*/                     // Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	//NVIC_EnableIRQ( DMA1_Channel2_IRQn );
	DMA1_Channel4->CFGR |= DMA_CFGR1_EN;

//	DMA1_Channel0->CNTR = DMA1_Channel4->CNTR; DMA1_Channel0->MADDR = DMA1_Channel4->MADDR; DMA1_Channel0->PADDR = DMA1_Channel4->PADDR; DMA1_Channel0->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel1->CNTR = DMA1_Channel4->CNTR; DMA1_Channel1->MADDR = DMA1_Channel4->MADDR; DMA1_Channel1->PADDR = DMA1_Channel4->PADDR; DMA1_Channel1->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel2->CNTR = DMA1_Channel4->CNTR; DMA1_Channel2->MADDR = DMA1_Channel4->MADDR; DMA1_Channel2->PADDR = DMA1_Channel4->PADDR; DMA1_Channel3->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel3->CNTR = DMA1_Channel4->CNTR; DMA1_Channel3->MADDR = DMA1_Channel4->MADDR; DMA1_Channel3->PADDR = DMA1_Channel4->PADDR; DMA1_Channel4->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel5->CNTR = DMA1_Channel4->CNTR; DMA1_Channel5->MADDR = DMA1_Channel4->MADDR; DMA1_Channel5->PADDR = DMA1_Channel4->PADDR; DMA1_Channel5->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel6->CNTR = DMA1_Channel4->CNTR; DMA1_Channel6->MADDR = DMA1_Channel4->MADDR; DMA1_Channel6->PADDR = DMA1_Channel4->PADDR; DMA1_Channel6->CFGR = DMA1_Channel4->CFGR;
	DMA1_Channel7->CNTR = DMA1_Channel4->CNTR; DMA1_Channel7->MADDR = DMA1_Channel4->MADDR; DMA1_Channel7->PADDR = DMA1_Channel4->PADDR; DMA1_Channel7->CFGR = DMA1_Channel4->CFGR;


	// TIM1_TRIG/TIM1_COM/TIM1_CH4 -> DMA1_Channel4

	// Remap, so PD4 is input for ETR
	AFIO->PCFR1 |=AFIO_PCFR1_TIM1_REMAP_1;

	funPinMode( PIN_PCK,   GPIO_CFGLR_IN_FLOAT );

	// BIG NOTE: 1<<15 can be set!!  (ETP) to change polarity.
	TIM1->SMCFGR = (7<<4) /* External trigger*/ | (0<<8) | (0<<7) | (0<<14) | (7) | (0<<12) | (1<<15) | (0<<14); // External
	TIM1->PSC = 0x0000;      // Prescaler 
	TIM1->ATRLR = 65535;       // Auto Reload - sets period
	TIM1->SWEVGR |= TIM_UG;	 // Reload immediately
	TIM1->DMAINTENR = (1<<15) | (1<<14) | (1<<13) | (1<<8) | (1<<5); // can trigger off of (1<<12)
	TIM1->CTLR1 |= TIM_CEN; // Enable
	TIM1->CNT = 0;
	TIM1->CTLR2 = 1<<2;

	// TIM2 full remap, to get output on PD6
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP;
	funPinMode( PIN_XCK,   GPIO_CFGLR_OUT_50Mhz_AF_PP );
	TIM2->PSC = 0x0000;      // Prescaler 
	TIM2->ATRLR = 1;       // Auto Reload - sets period
	TIM2->SWEVGR |= TIM_UG;	 // Reload immediately
	TIM2->CCER |= TIM_CC3E | TIM_CC3P;
	TIM2->CHCTLR2 |= TIM_OC1M_2 | TIM_OC1M_1;
	TIM2->CH3CVR = 1;
	TIM2->CTLR1 |= TIM_CEN; // Enable


	Delay_Ms( 1 );
	funDigitalWrite( PIN_RST, 1 );
	Delay_Ms( 1 );

	//Scan();

	ConfigureCamera();

	printf( "%d\n", (CamReadReg( 0x3804 ) << 8) | CamReadReg( 0x3805) );
	printf( "%d\n", (CamReadReg( 0x3806 ) << 8) | CamReadReg( 0x3807) );

	while( funDigitalRead( PD5 ) == 0 );// printf( "%02x", GPIOC->INDR );
	while( funDigitalRead( PD5 ) == 1 );// printf( "%02x", GPIOC->INDR );
	while( funDigitalRead( PD5 ) == 0 );// printf( "%02x", GPIOC->INDR );
	while( funDigitalRead( PD5 ) == 1 );// printf( "%02x", GPIOC->INDR );
//	while( funDigitalRead( PD5 ) == 0 ) printf( "%02x", GPIOC->INDR );
//	while( funDigitalRead( PD5 ) == 0 );
//	while( funDigitalRead( PD5 ) == 1 );
	DMA1_Channel4->CFGR = 0; // Disable DMA.

	printf( "Remain:%d/%d/%d/%d/%d/%d/%d / %d\n", 
		DMA1_Channel1->CNTR, DMA1_Channel2->CNTR,
		DMA1_Channel3->CNTR, DMA1_Channel4->CNTR,
		DMA1_Channel5->CNTR, DMA1_Channel6->CNTR,
		DMA1_Channel7->CNTR, 
		TIM1->CNT );
	int i;
	printf( "printf " );
	for( i = 0; i < sizeof(rawBuffer) - DMA1_Channel4->CNTR;i+=4)
	{
		char buffer[9];
		sprintf( buffer+0,"%02x", rawBuffer[i] );
		sprintf( buffer+2,"%02x", rawBuffer[i+1] );
		sprintf( buffer+4,"%02x", rawBuffer[i+2] );
		sprintf( buffer+6,"%02x", rawBuffer[i+3] );
		_write( 0, buffer, 8);
	}
	printf( " | xxd -r -p > test.jpg\n" );

	printf( "Chip: %02x%02x\n", CamReadReg( 0x300A ), CamReadReg( 0x300B ) );
	while(1)
	{
		Delay_Ms( 25 );
		printf( "%04x = %02x (%d) / %d\n", 0x4417, CamReadReg( 0x471D ), TIM1->CNT, DMA1_Channel4->CNTR );
	}
}

struct CamCmd
{
	uint16_t addy;
	uint8_t  data;
} __attribute__((packed));

int CamWriteReg( unsigned int addy, unsigned int data )
{
	int ac = 0;
	SendStart();
	if( SendByte( CAMERA_ADDRESS ) )
	{
		SendStop();
		return -1;
	}
	SendByte( addy >> 8 );
	SendByte( addy & 0xff );
	SendByte( data );
	SendStop();
	return ac;
}


int CamReadReg( unsigned int addy )
{
	SendStart();
	if( SendByte( CAMERA_ADDRESS ) )
	{
		SendStop();
		return -1;
	}
	SendByte( addy >> 8 );
	SendByte( addy & 0xff );
	SendStart();
	SendByte( CAMERA_ADDRESS | 1 );
	int ret = GetByte( 1 );
	SendStop();
	return ret;
}

// Learned a few things from https://github.com/espressif/esp32-camera/blob/4467667b71f22a4c7db226f24105a9350abe7a05/sensors/ov5640.c#L394

// Configure OV5640 over SCCB
void ConfigureCamera()
{
	#define CAMERA_ADDRESS 0x78
	const static struct CamCmd sccbs[] = {
		{0x3008, 0x80}, // Software reset
		{0x3008, 0x00}, // Unreset
		{0x3f00, 0x01}, // Reset MCU
		{0x3f00, 0x00}, // Unreset

		{0x3004, 0x00}, // All clocks off
		{0x3005, 0x00}, // 
		{0x3006, 0x00}, // 

		{0x3002, 0x00}, // SYSTEM RESET02 / Force system out of reest.

		// BIG NOTE: This seems to control the internal PLL for a variety of purposes!!
		{0x3034, 0x69}, // SC PLL CONTRL2 - default 0x69
		{0x3037, 0x03}, // SC PLL CONTRL3 - default 0x03
		{0x3039, 0x00}, // SC PLL CONTRL5 - default 0x00 -- setting to 0x80 bypasses PLL

		{0x3004, 0xff}, // CLOCK ENABLE00 - Enable BIST, MCU, OTP, STROBE, D5060, Timing and Array 
		{0x3005, 0xff}, // CLOCK ENABLE01 - Enable AWB, AFC, ISP, FC, S2P, BLC, AEC
		{0x3006, 0xff}, // CLOCK ENABLE02 - Enable PS, FMT, JPG, JPGx2, MUX, Avearge
		{0x3007, 0xe7}, // Enable all clocks, except MIPI

		// Neat! This slows down the pclk, which is useful for JPEG mode.
		{0x3108, 0x36}, // SYSTEM ROOT DIVIDER, (0x16) pll_clki/2 = default, switching to pll_clki/8 (0x36 seems to work)
//		{0x460C, 0x21}, // PCK Divisor override.
//		{0x3824, 0x1f}, // New divisor

		// TODO:
		//  It looks like there is a Y only mode to this. Look into that.
		{0x4300, 0x30}, // FORMAT CONTROL 00 YUV422
		{0x501F, 0x00}, // FORMAT MUX CONTROL (ISP YUV422)


		// Unknown stuff from ESP driver
		//unknown
		{0x370c, 0x02},//!!IMPORTANT
		{0x3634, 0x40},//!!IMPORTANT
		//isp control
		{0x5000, 0xa7},
		//{0x5001, 0xa3},//ISP CONTROL 01 -- enable scaling
		//{0x5003, 0x0a},//special_effect + bin enable
		// Same, without special
		{0x5001, 0x83},//ISP CONTROL 01 -- enable scaling
		{0x5003, 0x02},//special_effect + bin enable

		// No idea. this came from the ESP32 camera notes.
		// The author seems very sure that this is important
		// for some reason, and it sounds like there was a lot
		// of blood sweat and tears that went into it.
		//  "//0xd0 to 0x50 !!!"
		{0x471c, 0x50}, 

		{0x4740, 0x0C}, //POLARITY CTRL00 gate PCK

		//{0x4741, 0x00}, // Enable test pattern (Set to 0x07 for test pattern)

		// width/height control.
		{0x4602, 0},// 256
		{0x4603, 48},
		{0x4604, 0},
		{0x4605, 48},//192

		{0x3808, 0}, //TIMING DVPHO
		{0x3809, 48}, 
		{0x380A, 0}, //TIMING DVPHO
		{0x380B, 48},
		{0x3810, 0}, //TIMING DVPHO
		{0x3811, 0}, 
		{0x3812, 0}, //TIMING DVPHO
		{0x3813, 0}, 

		{0x5600, 0x3f}, // Scale (/16, /16)
		{0x5601, 0x33}, // Scale (/8, /8)

		// Full size is 2592 x 1944
		{0x3800, 6}, //TIMING DVPHO This is an offset
		{0x3801, 0}, 
		{0x3802, 4}, //TIMING DVPHO This is an offset
		{0x3803, 0}, 

		{0x4407, 0x2c}, // JPEG Quality https://community.st.com/t5/stm32-mcus-embedded-software/ov5640-jpeg-compression-issue-when-storing-images-on-sd-card/td-p/663684

		{0x3017, 0x7f},  // Pad output control, FREX = 0, vsync, href, pclk outputs. D9:6 enable.
		{0x3018, 0xfc},  // Pad output enable, D5:0 = 1.  GPIO0/1 = off.

		{0x4404, 0x34}, // JPEG CTRL04, normally 0x24, But, Enable gated clock = 1
		{0x4713, 0x01}, // JPEG mode (Default 2)
		//{0x4600, 0}, // VFIFO CTRL00 0x00
		{0x3821, 0x20}, // COMPRESSION ENABLE ... by default 0x00, and BINNING MODE set to true.
	};


		//{0x4837, 0xff}, // PCK Period (Does not appear to do anything) (Seems to be MIPI only) (DOES NOTHING)
		//{0x470b, 0x0f},  //VSYNC WIDTH PCLK UNIT -- does this matter, by default 0x01 (DOES NOTHING?)


	int i;
	struct CamCmd * cmd = sccbs;
	for( i = 0; i < sizeof(sccbs) / sizeof(sccbs[0]); i++ )
	{
		uint16_t addy = cmd->addy;
		uint8_t  val = cmd->data;
		if( CamWriteReg( addy, val ) ) goto fail;
		cmd++;
	}

	return;
fail:
	printf( "Fail on cmd %d\n", i );
}
