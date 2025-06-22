#include "ch32fun.h"
#include <stdio.h>

#define RANDOM_STRENGTH 2

#include "lib_rand.h"

void ConfigureCamera();
int CamReadReg( unsigned int addy );
int CamWriteReg( unsigned int addy, unsigned int data );

#define MAX_INTENSITY 255

uint8_t LEDSets[9*8];
const uint16_t Coordmap[8*16] = {
	0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0xffff, 0xffff,
	0x0002, 0x0102, 0x0202, 0x0302, 0x0402, 0x0502, 0xffff, 0xffff,
	0x0001, 0x0101, 0x0201, 0x0301, 0x0401, 0x0501, 0xffff, 0xffff,
	0x0008, 0x0108, 0x0208, 0x0308, 0x0408, 0x0508, 0xffff, 0xffff,
	0x0007, 0x0107, 0x0207, 0x0307, 0x0407, 0x0507, 0xffff, 0xffff,
	0x0006, 0x0106, 0x0206, 0x0306, 0x0406, 0x0506, 0xffff, 0xffff,
	0x0005, 0x0205, 0x0405, 0x0605, 0x0600, 0x0700, 0xffff, 0xffff,
	0x0004, 0x0204, 0x0404, 0x0604, 0x0602, 0x0702, 0xffff, 0xffff,
	0x0003, 0x0203, 0x0403, 0x0603, 0x0601, 0x0701, 0xffff, 0xffff,
	0x0105, 0x0305, 0x0505, 0x0705, 0x0608, 0x0708, 0xffff, 0xffff,
	0x0104, 0x0304, 0x0504, 0x0704, 0x0607, 0x0707, 0xffff, 0xffff,
	0x0103, 0x0303, 0x0503, 0x0703, 0x0606, 0x0706, 0xffff, 0xffff,
};

void SetPixel( int x, int y, int intensity )
{
	int i;

	int coord = Coordmap[x*8+y];

	int ox = coord & 0xff;
	int oy = coord >> 8;

	int ofs = ox;
	uint8_t * ledo = &LEDSets[ofs];
	int imask = ~(1<<oy);
	int mask = ~imask;

	for( i = 0; i < 8; i++ )
	{
		if( intensity & (1<<i) )
			*ledo |= mask;
		else
			*ledo &= imask;
		ledo += 9;
	}

#if 0
	for( i = MAX_INTENSITY-1; i >= intensity; i-- )
	{
		*ledo &= imask;
		ledo += 9;
	}
	for( ; i >= 0; i-- )
	{
		*ledo |= mask;
		ledo += 9;
	}
#endif
}

int apsqrt( int i )
{
	if( i == 0 ) return 0;
	int x = 1<<( ( 32 - __builtin_clz(i) )/2);
	x = (x + i/x)/2;
//	x = (x + i/x)/2; //May be needed depending on how precise you want. (Below graph is without this line)
	return x;
}

int main()
{
	SystemInit();

	// Enable DMA
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;

	// Enable timers and remapping.
	RCC->APB2PCENR = RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
	RCC->APB1PCENR = RCC_APB1Periph_TIM2;

	//Configure most IOs ar output.
	GPIOC->CFGLR = 0x22222222; // GPIO_CFGLR_IN_FLOAT = 4, 
	GPIOD->CFGLR = 0x22222242; // GPIO_CFGLR_OUT_2Mhz_PP = 2
	GPIOA->CFGLR = 0x220; // GPIO_CFGLR_OUT_2Mhz_PP = 2

	// Configure default output values.
	GPIOA->BSHR = 6;
	GPIOD->BSHR = 0xfd;

	//static uint8_t As[] = { PD0, PA1, PA2, PD2, PD3, PD4, PD5, PD6, PD7 };
	static volatile uint32_t * RowControls[] = { &GPIOD->OUTDR, &GPIOA->OUTDR, &GPIOA->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR };
	static uint8_t    RowValue[] = { ~0x01, 0xff, ~0x02, 0xff, ~0x04, 0xff, ~0x04, 0xff, ~0x08, 0xff, ~0x10, 0xff, ~0x20, 0xff, ~0x40, 0xff, 0x7f,  0xff };

	#define CLONESET(x)   x, x, x, x, x, x, x, x, x
	#define CLONESETM2(x) x, x, x, x, x, x, x
	static uint16_t    PrescaleChange[] = { 
		CLONESETM2(1), CLONESET(3), CLONESET(7), CLONESET(15), CLONESET(31), CLONESET(63), CLONESET(127), CLONESET(255), 1, 1
	};
	//3, 7, 15, 31, 63, 127, 255, 511 };

	// T2C3 is the DMA->DMA Control, DMAC1.
	//	  DMAC1 reads from RowControls and sets the OUTDR for DMAC7.
	// T1C2/C4 is the DMA->ROW Control, DMAC7
	//    This trigegrs 2x as fast as the others, and sets a row.
	// T2C1 is the DMA->PC Control, DMAC5
	//    This outputs to a Column.  I.e. Port C.
	// T2U is the timer to change the timer speed in DMAC2

	DMA1_Channel1->CNTR = 9;
	DMA1_Channel1->MADDR = (uint32_t)&RowControls[0];
	DMA1_Channel1->PADDR = (uint32_t)&DMA1_Channel7->PADDR;
	DMA1_Channel1->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL /* Highest Priority */ | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN | DMA_CFGR1_MSIZE_1 | DMA_CFGR1_PSIZE_1;

	DMA1_Channel7->CNTR = 18;
	DMA1_Channel7->MADDR = (uint32_t)&RowValue[0];
	//DMA1_Channel3->PADDR = // Configured by DMAC5
	DMA1_Channel7->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL_0 | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN /* 8-bit in, 8-bit out */;

	DMA1_Channel5->CNTR = sizeof(LEDSets);
	DMA1_Channel5->MADDR = (uint32_t)LEDSets;
	DMA1_Channel5->PADDR = (uint32_t)&GPIOC->OUTDR;
	DMA1_Channel5->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL_0 | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN /* 8-bit in, 8-bit out */;

	DMA1_Channel2->CNTR = sizeof(PrescaleChange)/sizeof(PrescaleChange[0]);
	DMA1_Channel2->MADDR = (uint32_t)PrescaleChange;
	DMA1_Channel2->PADDR = (uint32_t)&TIM2->PSC;
	DMA1_Channel2->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL /* Highest Priority*/ | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN | DMA_MemoryDataSize_HalfWord | DMA_PeripheralDataSize_HalfWord /* 8-bit in, 8-bit out */;

	TIM2->PSC = 0x4;      // Prescaler  (Fastest we can reliably go without race conditions) (3 works _almost_ all the time, 4 is always reliable)
	TIM2->ATRLR = 20;       // Auto Reload - sets period  (This is how fast each pixel works per set)
	TIM2->SWEVGR = TIM_UG;	 // Reload immediately
	TIM2->DMAINTENR = TIM_COMDE | TIM_TDE | TIM_UDE | TIM_CC2DE | TIM_CC1DE | TIM_CC3DE | TIM_CC4DE;
	TIM2->CH1CVR = 2;
	TIM2->CH2CVR = 4;
	TIM2->CH3CVR = 1;
	TIM2->CH4CVR = 20; // This must be ATLAR.
	TIM2->CNT = 0;
	TIM2->CTLR1 |= TIM_CEN; // Enable

	int frameno = 0;
	#define CONCURRENT_DROPS 3
	#define DROPLIFE 1200
	#define SEPARATION 2

	int droptime[CONCURRENT_DROPS];
	int droplife[CONCURRENT_DROPS];
	int dropx[CONCURRENT_DROPS];
	int dropy[CONCURRENT_DROPS];

	int i;
	for( i = 0; i < CONCURRENT_DROPS; i++ )
	{
		droptime[i] = DROPLIFE * i / CONCURRENT_DROPS;
		dropx[i] = rand() % 12;
		dropy[i] = rand() % 6;
		droplife[i] = rand() % 100;
	}


	while(1)
	{
		int y, x, i;
		int d;
		uint8_t framebuffer[12*6];
#if 1 // Raindrops animation
		for( d = 0; d < CONCURRENT_DROPS; d++ )
		{
			i = 0;
			for( y = 0; y < 6; y++ )
			for( x = 0; x < 12; x++ )
			{
				int usex = (x < 6)?x:(x+SEPARATION);
				int dx = usex - dropx[d];
				int dy = y - dropy[d];
				int apd = ((apsqrt( dx*dx*MAX_INTENSITY*64+dy*dy*MAX_INTENSITY*64 ) - droptime[d]*(MAX_INTENSITY/64)) ) + MAX_INTENSITY;
				if( apd < 0 ) apd = -apd;
				int inten = (MAX_INTENSITY - apd - 1);
				if( inten < 0 ) inten = 0;
				inten += framebuffer[i];
				if( inten >= MAX_INTENSITY ) inten = MAX_INTENSITY-1;
				framebuffer[i] = inten;
				i++;
				//SetPixel( x, y, ((x*2142*y*2472+fxo)>>8)&(MAX_INTENSITY-1));
				
			}

			droptime[d]++;
			droplife[d]--;
			if( droplife[d] < 0 )
			{
				droptime[d] = 0;
				droplife[d] = (rand() % 220)+700;
				dropx[d] = rand() % (12+SEPARATION);
				dropy[d] = rand() % 6;
			}
		}

		i = 0;
		for( y = 0; y < 6; y++ )
		for( x = 0; x < 12; x++ )
		{
			SetPixel( x, y, framebuffer[i] );
			framebuffer[i] = 0;
			i++;
		}
		//Delay_Ms(5);

#else // Gradient

		i = 0;
		for( y = 0; y < 6; y++ )
		for( x = 0; x < 12; x++ )
		{
			SetPixel( x, y, x+y*12 ); 
			//(x+y*31)&0xff );
		}
		Delay_Ms(5);

#endif

/*
		int y, x;
		for( y = 0; y < 9; y++ )
		for( x = 0; x < 8; x++ )
		{
			funDigitalWrite( PC0 + x, 1 );
			funDigitalWrite( As[y], 0 );
			Delay_Us(10);
			funDigitalWrite( As[y], 1 );
			funDigitalWrite( PC0 + x, 0 );
		}
*/
		frameno++;
	}
}
