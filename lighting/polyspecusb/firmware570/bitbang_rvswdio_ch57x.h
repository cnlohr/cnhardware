// Implementation of platform specific functions in bitbang_rvswdio.h
//
// If you want the most up-to-date version of this, it will be stored in rv003usb.
//

#define SWD_DELAY ADD_N_NOPS( 5 );

#ifndef _BITBANG_RVSWDIO_CH570_H
#define _BITBANG_RVSWDIO_CH570_H

static inline void Delay_Tiny_Inline( int n ) {
	asm volatile( "\
		1: \
		c.addi %[n], -1\n\
		c.bnez %[n], 1b" : [n]"+r"(n) );
}

static inline void Send1Bit(void) __HIGH_CODE;
static inline void Send0Bit(void) __HIGH_CODE;
static inline int ReadBit(void) __HIGH_CODE;
static void FinishBuffer(void);
static void IssueBuffer(void);

void ConfigureIOForRVSWD(void);
void ConfigureIOForRVSWIO(void);

static inline void Send1Bit(void)
{
	// 288-296ns low pulse. (Ideal period) - OK @ 2..15
	R32_PA_DIR |= PIN_SWD;
	R32_PA_CLR = PIN_SWD;
	Delay_Tiny_Inline( 5 ); // Valid (no end break) 1 (100ns) .. 9 (500ns)
	R32_PA_SET = PIN_SWD;
	Delay_Tiny_Inline( 16 ); // Valid 0+
}

static inline void Send0Bit(void)
{
	// 888-904ns - OK @ 32 (At TPERIOD=48) - @48 (At TPERIOD=56)
	// Oddly, also works at =20 for TPERIOD=56
	R32_PA_DIR |= PIN_SWD;
	R32_PA_CLR = PIN_SWD;
	Delay_Tiny_Inline( 16 ); // Valid 10 (500ns) .. 40 (2us)
	R32_PA_SET = PIN_SWD;	
	Delay_Tiny_Inline( 16 ); // Valid 0+
}

static inline int ReadBit(void)
{

	R32_PA_CLR = PIN_SWD;
	Delay_Tiny_Inline( 2 );
	R32_PA_DIR &= ~PIN_SWD;
	Delay_Tiny_Inline( 5 );
	int r = !!(R32_PA_PIN&PIN_SWD);
	R32_PA_SET = PIN_SWD;
	Delay_Tiny_Inline( 12 );
	R32_PA_DIR |= PIN_SWD;
	return r;
#if 0
	uint32_t ret;
	asm volatile( "\n\
		lui a2, 0x40001\n\
		ori a2, a2, 0x0a0 /*R32_PA_DIR*/\n\
		lw a3, 0(a2) /* old DIR = a3 */ \n\
		andi a4, a3, 0b11111011\n\
		li a0, 0b100\n\
		sw a0, (0xac-0xa0)(a2) /* R32_PA_CLR */\n\
	/*	li a1, 1\n 1: c.addi a1, -1\n  c.bnez a1, 1b */\n\
		sw a4, (0xa0-0xa0)(a2) /* R32_PA_DIR release */\n\
		sw a0, (0xb8-0xa0)(a2) /* R32_PA_SET */\n\
	/*	sw a3, (0xa0-0xa0)(a2) */ /* R32_PA_DIR drive for one */\n\
	/*	sw a4, (0xa0-0xa0)(a2) */ /* R32_PA_DIR release */\n\
		li a1, 25\n 1: c.addi a1, -1\n  c.bnez a1, 1b\n\
		lw %[ret], (0xa4-0xa0)(a2) /* R32_PA_PIN */\n\
		li a1, 10\n 1: c.addi a1, -1\n  c.bnez a1, 1b\n\
		sw a3, (0xa0-0xa0)(a2) /* R32_PA_DIR drive */\n\
		" : [ret]"=r"(ret) : : "a0", "a1", "a2", "a3", "a4" );
	return !!ret;
#endif



}


/*	uint32_t odir = R32_PA_DIR;
	uint32_t nod = odir & ~PIN_SWD;
	R32_PA_CLR = PIN_SWD;
	Delay_Tiny_Inline( 2 );
	R32_PA_SET = PIN_SWD;
	R32_PA_DIR = nod;
	Delay_Tiny_Inline( 5 );
	int r = !!(R32_PA_PIN&PIN_SWD);
	R32_PA_SET = PIN_SWD;
	Delay_Tiny_Inline( 16 );
	R32_PA_DIR = odir;
	return r;*/

static void IssueBuffer(void) { }

static void FinishBuffer(void)
{
	__enable_irq();
}

////////////////////////////////////////////////////////////////
// For SWD
////////////////////////////////////////////////////////////////

static void SendBitRVSWD( int val )
{
	// Assume:
	// SWD is in indeterminte state.
	// SWC is HIGH
	funDigitalWrite( PIN_SWC, 0 );
	if( val )
	{
		funDigitalWrite( PIN_SWD, 1 );
	}
	else
	{
		funDigitalWrite( PIN_SWD, 0 );
	}
	funPinMode( PIN_SWD, GPIO_CFGLR_OUT_10Mhz_PP );
	SWD_DELAY;
	funDigitalWrite( PIN_SWC, 1 );
	SWD_DELAY;
}

static int ReadBitRVSWD( void )
{
	funPinMode( PIN_SWD, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PIN_SWD, 1 );
	funDigitalWrite( PIN_SWC, 0 );
	SWD_DELAY;
	int r = !!(funDigitalRead( PIN_SWD ));
	funDigitalWrite( PIN_SWC, 1 );
	SWD_DELAY;
	return r;
}

static void RVFinishRegop(void)
{
	ReadBitRVSWD( ); // ???
	ReadBitRVSWD( ); // ???
	ReadBitRVSWD( ); // ???
	SendBitRVSWD( 1 ); // 0 for register, 1 for value
	SendBitRVSWD( 0 ); // ??? Seems to have something to do with halting?


	funDigitalWrite( PIN_SWC, 0 );
	SWD_DELAY;
	funDigitalWrite( PIN_SWD, 0 );
	funPinMode( PIN_SWD, GPIO_CFGLR_OUT_50Mhz_PP );
	SWD_DELAY;
	funDigitalWrite( PIN_SWC, 1 );
	SWD_DELAY;
	funDigitalWrite( PIN_SWD, 1 );

	Delay_Us(2); // Sometimes 2 is too short.
	__enable_irq();
}

static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	__disable_irq();
	if( state->opmode == 1 )
	{
		//printf( "WRITEREG: %02x %08x\n", command, value );
		Send1Bit();
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1Bit();
			else
				Send0Bit();
		}
		Send1Bit( );
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			if( value & mask )
				Send1Bit();
			else
				Send0Bit();
		}

		IssueBuffer();
		FinishBuffer();
	}
	else if( state->opmode == 2 )
	{
		uint32_t mask;
		funDigitalWrite( PIN_SWD, 0 );
		SWD_DELAY;
		int parity = 1;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( 1 ); // Write = Set high
		SendBitRVSWD( parity );
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // Seems only need to be set for first transaction (We are ignoring that though)
		ReadBitRVSWD( ); // ???
		SendBitRVSWD( 0 ); // 0 for register, 1 for value.
		SendBitRVSWD( 0 ); // ???  Seems to have something to do with halting.

		parity = 0;
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			int v = !!(value & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( parity );
		RVFinishRegop();
	}
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	__disable_irq();
	if( state->opmode == 1 )
	{
		Send1Bit();
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1Bit();
			else
				Send0Bit();
		}
		Send0Bit( );
		uint32_t rv = 0;
		int i;
		for( i = 0; i < 32; i++ )
		{
			rv = (rv << 1) | ReadBit();
		}
		ReadBit(); // check bit.
		IssueBuffer();
		FinishBuffer();

		memcpy( value, &rv, 4 );
	}
	else if( state->opmode == 2 )
	{
		int mask;
		funDigitalWrite( PIN_SWD, 0 );
		SWD_DELAY;
		int parity = 0;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( 0 ); // Read = Set low
		SendBitRVSWD( parity );
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // ???
		SendBitRVSWD( 0 ); // 0 for register, 1 for value
		SendBitRVSWD( 0 ); // ??? Seems to have something to do with halting?


		uint32_t rval = 0;
		int i;
		parity = 0;
		for( i = 0; i < 32; i++ )
		{
			rval <<= 1;
			int r = ReadBitRVSWD( );
			if( r == 1 )
			{
				rval |= 1;
				parity ^= 1;
			}
			if( r == 2 )
			{
				RVFinishRegop();
				return -1;
			}
		}
		memcpy( value, &rval, 4 );

		if( ReadBitRVSWD( ) != parity )
		{
			//BB_PRINTF_DEBUG( "Parity Failed\n" );
			RVFinishRegop();
			return -1;
		}

		RVFinishRegop();
	}
	return 0;
}

void ConfigureIOForRVSWD(void)
{
	funDigitalWrite( PIN_SWC, 1 );
	funPinMode( PIN_SWC, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PIN_SWD, 1 );
	funPinMode( PIN_SWD, GPIO_CFGLR_OUT_10Mhz_PP );
}

void ConfigureIOForRVSWIO(void)
{
	funDigitalWrite( PIN_SWD, 1 );
	funPinMode( PIN_SWD, GPIO_CFGLR_OUT_10Mhz_PP );
}

#endif


