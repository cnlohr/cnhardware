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

// There are other modes I'm finding where VS is more useful than HS.
#define PIN_VS    PD2 //  THIS IS USED
#define PIN_HS    PD5 //  THIS IS IGNORED
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

void SetupDMA();

void WaitToggle()
{
	while( funDigitalRead( PIN_VS ) == 1 );// printf( "%02x", GPIOC->INDR );
	while( funDigitalRead( PIN_VS ) == 0 );// printf( "%02x", GPIOC->INDR );
//	while( funDigitalRead( PD5 ) == 0 );// printf( "%02x", GPIOC->INDR );
//	while( funDigitalRead( PD5 ) == 1 );// printf( "%02x", GPIOC->INDR );
}

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



	SetupDMA();

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

	WaitToggle();
	SetupDMA(); // really wait for next frame.
	WaitToggle();
	int frameno = 0;

	printf( "Chip: %02x%02x\n", CamReadReg( 0x300A ), CamReadReg( 0x300B ) );
	while(1)
	{
		int cntrl = DMA1_Channel4->CNTR;
		DMA1_Channel4->CFGR = 0; // Disable DMA.

		printf( "%ld / %d\n", DMA1_Channel4->CNTR, TIM1->CNT );

		int i;
		printf( "printf " );
		if( frameno == 0 || rawBuffer[1] != 0xff ) printf( "ff" );
		// Skip first byte just because
		for( i = 1; i < sizeof(rawBuffer) - cntrl;i+=4)
		{
			char buffer[9];
			sprintf( buffer+0,"%02x", rawBuffer[i] );
			sprintf( buffer+2,"%02x", rawBuffer[i+1] );
			sprintf( buffer+4,"%02x", rawBuffer[i+2] );
			sprintf( buffer+6,"%02x", rawBuffer[i+3] );
			_write( 0, buffer, 8);
		}
		printf( " | xxd -r -p > test.jpg\n" );
		WaitToggle();
		SetupDMA();
		//Delay_Ms( 25 );
		//printf( "%04x = %02x (%d) / %d\n", 0x4417, CamReadReg( 0x471D ), TIM1->CNT, DMA1_Channel4->CNTR );
		WaitToggle();
		frameno++;
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


#define REG16( x, v ) { (x), (v)>>8 }, { (x)+1, (v)&0xff }

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

		{0x3002, 0x1c}, // SYSTEM RESET02 / Force system out of reest.
		{0x3002, 0x00}, // SYSTEM RESET02 / Force system out of reest.

		// BIG NOTE: This seems to control the internal PLL for a variety of purposes!!
		{0x3034, 0x10}, // SC PLL CONTRL0  This one is tricky: Bottom nibble claims MIPI stuff, but if I sent it higher things get wacky. 
		{0x3035, 0x2f}, // SC PLL CONTRL1 - This actually controls our PCLK. (or not? I am confused)
		{0x3036, 0x69}, // was 0x69 Main system PLL Speed ??? dunno what it is but it seems stable.
		{0x3037, 0x03}, // SC PLL CONTRL3 - default 0x03
		{0x3039, 0x00}, // SC PLL CONTRL5 - default 0x00 -- setting to 0x80 bypasses PLL

		{0x3004, 0xff}, // CLOCK ENABLE00 - Enable BIST, MCU, OTP, STROBE, D5060, Timing and Array 
		{0x3005, 0xf7}, // CLOCK ENABLE01 - Enable AWB, AFC, ISP, FC, NO S2P, BLC, AEC
		{0x3006, 0xff}, // CLOCK ENABLE02 - Enable PS, FMT, JPG, JPGx2, MUX, Avearge
		{0x3007, 0xe7}, // Enable all clocks, except MIPI

		// Neat! This slows down the pclk, which is useful for JPEG mode.
		{0x3108, 0x36}, // SYSTEM ROOT DIVIDER, (0x16) pll_clki/2 = default, switching to pll_clki/8 (0x36 seems to work)
		{0x460C, 0xa3}, {0x3824, 0x02}, // PCK Divisor override.  // this causes corruption if you slow it down.
		//{0x460D, 0xff},

		{0x3103, 0x03}, // System clock input = PLL, Some things note that it should really be source 3?

		// The following are mentioned here, they don't seem to cause anything
		// https://github.com/xenpac/sun4i_csi/blob/master/device/ov5640.c
		{0x300e, 0x58}, // Default DVP mode
		{0x460b, 0xf4}, // Some sort of DVP setting (Was 0x35, but 0xf4 works better)
		{0x4837, 0xff}, // (DOES NOTHING): PCK Period (Does not appear to do anything) (Seems to be MIPI only) 
		{0x302e, 0x00}, // ?? according to them?  I see no pattern of change here.
		{0x3001, 0x08}, // Enable some blocks, no s2p.
		{0x5025, 0x00}, // ??

		// scale2 settings?  dunno what it means But it does look visually nicer.
		{0x3618, 0x00},
		{0x3612, 0x29},
		{0x3709, 0x52},
		{0x370c, 0x03}, 

		// 0x43xx are FORMAT registers, appear to not be in use in JPEG mode.
		{0x4300, 0x30}, // FORMAT CONTROL 00 YUV422 (Does not seem to control for JPEG mode)
		//REG16( 0x4302, 0x7ff ), //YMAX
		//REG16( 0x4304, 0x00 ), //YMIN
		//REG16( 0x4306, 0x7ff ), //UMAX
		//REG16( 0x4308, 0x00 ), //UMIN
		//REG16( 0x430a, 0x7ff ), //VMAX
		//REG16( 0x430c, 0x00 ), //VMIN
		{0x501F, 0x00}, // FORMAT MUX CONTROL (ISP YUV422)
		//{0x501E, 1<<6}, // Scale manual enable.  Does nothing?
		//{0x501D, 1<<4}, // Scale manual enable. Does nothing?

		// Unknown stuff from ESP driver
		//unknown
		//{0x370c, 0x02},//!!IMPORTANT
		//{0x3634, 0x40},//!!IMPORTANT
		// These don't seem important.
		//isp control
		{0x5000, 0x87}, // was 0x07, but suni4_csi says make it a7
		{0x5001, 0x23},//ISP CONTROL 01 -- enable scaling + (Disable SDE)
		{0x5003, 0x04},// bin enable

		//  If you turn on SDE you can do things like this,
		// to use this, 0x5001 must be |0x80
		{0x5580, 0x02}, // Special effects, Saturation enable.


		// subsampling Making these smaller will subsample from a larger
		// image but will take longer. ... or not this is weird.
		// Setting to 88 will cause weird artifacts.
		// BIG NOTE: increasing this to 71 71 will help
		// speed up ISP and JPEG, for higher framerate.
		{0x3814, 0x71},
		{0x3815, 0x71},

		// default is 0x01,
		// 0x00 in jpeg makes it so vsync isn't going crazy in the middle.
		// 0x02 in jpeg makes it so vsync becomes a notch right before the jpeg data.
		// HIDDEN MODES: Just found out you can shrink vsync by setting
		{0x471D, 0x05},
 
		//{0x471F, 0x30}, // DVP HREF CTRL HREF Minimum Blanking in JPEGMode23 (0x40 default) Not useful.
		//{0x471e, 0x10}, // DVP HREF CTRL HREF Minimum Blanking in JPEGMode23 (0x40 default) Not useful.
		//{0x4723, 0x02}, // Skip line number (Doesn't seem to do anything)
		//{ 0x4730, 0xff }, // Clip data disable. ?? Doesn't do anything? default is 0.

		// No idea. this came from the ESP32 camera notes.
		// The author seems very sure that this is important
		// for some reason, and it sounds like there was a lot
		// of blood sweat and tears that went into it.
		//  "//0xd0 to 0x50 !!!"
		// It seems to squeeze some of the jpeg things closer together to be more
		// chaotic, but in our case that's probably a good thing.
		{0x471c, 0xd0}, 

		//{0x4741, 0x00}, // Enable test pattern (Set to 0x07 for test pattern)

		// width/height control.
		REG16( 0x4602, 192 ), //JPEG VFIFO HSIZE
		REG16( 0x4604, 128 ), //JPEG VFIFO VSIZE

		REG16( 0x3808, 192 ), // X_OUTPUT_SIZE (ISP output)
		REG16( 0x380A, 128 ), // Y_OUTPUT_SIZE (ISP output)

		// A mode mentioned in espressif's example
	    //    mw,   mh,  sx,  sy,   ex,   ey, ox, oy,   tx,   ty
		//    1920, 1920, 320,   0, 2543, 1951, 32, 16, 2684, 1968 }, //1x1

		// Full size is 2592 x 1944

		// Original
		REG16( 0x3800, 0 ), // X_ADDR_ST
		REG16( 0x3802, 0 ), // Y_ADDR_ST
		REG16( 0x3804, 2623 ), // X_ADDR_END
		REG16( 0x3806, 1951 ), // Y_ADDR_END
		REG16( 0x380c, 1344 ), //  Decreasing this  squeezes the frame time-wise but is harder to deal with.
		REG16( 0x380e, 650 ), // 
		REG16( 0x3810, 0 ), // X_OFFSET (inside of window offset)
		REG16( 0x3812, 0 ), // Y_OFFSET (inside of window offset)

 	// These seem to do nothing.
	//	REG16( 0x3816, 0 ), // HSYNC Start point.
 	//	REG16( 0x3818, 16 ), // HSYNC Width (Doesn't seem to do anything)
 //		{0x4721, 0x0f},
 //		{0x4722, 0x0f},		
 //		{0x471f, 0x0f},		
 //		{0x4722, 0x0f},		
 //		{0x471b, 0x01}, //HSYNC mode enable?

		// VSYNC width PCLK unit, does nothing in this mode?
		//REG16( 0x470A, 2048 ),
		//{0x4711, 0x80 }, // PAD LEFT CTRL // Does nothing?

		// Test image. (This one works)
		//{0x503D, 0xc0},


		{0x501D, 1<<4}, //ISP MISC (Average size manual enable)

		{0x5600, 0x1a}, // Scale control, 0x10 is OK, but 0xff looks better?  Though it is dropping. Tweak me.
		// If you don't average the image quality is potato 0xfa = average 0xff = no.
		{0x5601, 0x33}, // Scale (/16, /16) = 0x55  --- This seems to not do anything in some cases?

		// Curiously when the above is not 0x55, the JPEG engine seems to accumulate errors.

		//{0x5601, 0x00}, // Scale (/1, /1)
		// Other scale stuff seems not to do anything.
		REG16( 0x5602, 14 ),
		REG16( 0x5604, 14 ),

		REG16( 0x3500, 0x07ff ),// Exposure?
		REG16( 0x350a, 0x01ff ),// Gain
		{ 0x3503, 0x03},// Auto enable.


		// XXX TODO see if this makes things more reliable.
		// This can slow down internal JPEG read speeds
		{0x4400, 0x81}, // Other speeds control.  Does not seem useful
		{0x4401, 0x01}, // Other speeds control.  Does not seem useful

		// 0x7f is the lowest possible quality.
		{0x4407, 0x1f}, // JPEG Quality https://community.st.com/t5/stm32-mcus-embedded-software/ov5640-jpeg-compression-issue-when-storing-images-on-sd-card/td-p/663684

		// Do we want FREX?
		{0x3017, 0xff},  // Pad output control, FREX = 0, vsync, href, pclk outputs. D9:6 enable.
		{0x3018, 0xfc},  // Pad output enable, D5:0 = 1.  GPIO0/1 = off.

		// 2 is default and ok, 1 gets weird clocky
		{0x4713, 0x02}, // JPEG mode (Default 2)

		// very important.  This allows dynamic sizing
		{0x4600, 0x80}, // VFIFO CTRL00 0x00 default - but we can set it to Compression output fixedheight enable = true if in mode 2, it seems more relaible

		{0x3821, 0x23}, // JPEG +  (And mirror) / lsb = Horizontal Bin enable = true.

		// This gates the pclk with href/vref
		{0x4740, 0x0c}, //POLARITY CTRL00 gate PCK

		{0x4709, 0x01}, // Shrink Vsync
		//{0x470a, 0x00}, // no impact
		//{0x470b, 0x00}, // no impact
		// Does not seem to have an impact.
		//{0x440a, 0x01}, //JFIF output delay.

		// What does gated clock do?  It doesn't seem to do anything? -- even in conjunction with 
		{0x4404, 0x34}, // enable gated clock, and this is where you turn the header on and off.
	};


		//{0x4837, 0xff}, // PCK Period (Does not appear to do anything) (Seems to be MIPI only) (DOES NOTHING)
		//{0x470b, 0x0f},  //VSYNC WIDTH PCLK UNIT -- does this matter, by default 0x01 (DOES NOTHING?)

		// These seem to do nothing
		//{0x4404, 0x34}, // JPEG CTRL04, normally 0x24, But, Enable gated clock = 1
		//{0x440b, 0xf0}, //Slow end of dummy down.
		//{0x440c, 0xff}, //Slow end of dummy down.
		//{0x4400, 0x88}, // Mode/JFIFO read speed **** Important!!  We can slow the JPEG down.
		//{0x4401, 0xff}, // Other speeds control.  Does not seem useful
		//{0x4402, 0x94},


	int i;
	const struct CamCmd * cmd = sccbs;
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


void SetupDMA()
{
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
		/*DMA_CFGR1_CIRC |*/                     // Circular mode.
		/*DMA_CFGR1_HTIE |*/                    // Half-trigger
		/*DMA_CFGR1_TCIE |*/                     // Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	//NVIC_EnableIRQ( DMA1_Channel2_IRQn );
	DMA1_Channel4->CFGR |= DMA_CFGR1_EN;
	TIM1->CNT = 0;
}


