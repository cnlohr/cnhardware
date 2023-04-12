//Copyright 2012-2013 <>< Charles Lohr
//This file may be used for any purposes (commercial or private) just please leave this copyright notice in there somewhere.

//Generic I2C static library for AVRs (ATMega and ATTinys) (Commented out portion)
//Now, generic I2C library for any GPIO-based system.

//Include this in your .c file only!!!
//Include it after the following are defined:

// DSDA -> Pin number the SDA line is on.
// DSCL -> Pin number the SCL line is on.
//
// DPORTNAME -> Port name, i.e. A, B, C, D
//
// I2CDELAY_FUNC -> If you wish to override the internal delay functions.
// I2CSPEEDBASE -> define speed base multiplier 1 = normal, 10 = slow, .1 = fast; failure to define this will result in the clock being as fast as possible.
// I2CNEEDGETBYTE -> Do we need to be able to read data?
//
// I2CPREFIX   -> #define to be the prefix, i.e. BOB will cause  BOBConfigI2C to be generated.
// I2CNOSTATIC -> #define if you want the functions to be generated as not-static code.
//
//NOTE: You must initially configure the port to be outputs on both DSDA and DSCL and set them both to be driven high.

#ifndef I2CPREFIX
#define I2CPREFIX
#endif

#ifndef I2CNOSTATIC
#define I2CSTATICODE 
#else
#define I2CSTATICODE static
#endif

#ifndef I2CFNCOLLAPSE
#define INTI2CFNCOLLAPSE( PFX, name ) PFX ## name
#define I2CFNCOLLAPSE( PFX, name ) INTI2CFNCOLLAPSE( PFX, name )
#endif

#define DDDR I2CFNCOLLAPSE( DDR, DPORTNAME )
#define DPIN I2CFNCOLLAPSE( PIN, DPORTNAME )
#define DPORT I2CFNCOLLAPSE( PORT, DPORTNAME )

#ifndef I2CNEEDGETBYTE
#define I2CNEEDGETBYTE 1
#endif

#ifndef I2CDELAY_FUNC
#ifdef I2CSPEEDBASE
#define I2CDELAY_FUNC(x) os_delay_us(x)
#else
#define I2CDELAY_FUNC(x) 
#endif
#else

#endif


//Default state:
//PORT LOW
//DDR  Undriven

I2CSTATICODE void I2CFNCOLLAPSE( I2CPREFIX, ConfigI2C ) ()
{
	PIN_DIR_INPUT = _BV(DSDA) | _BV(DSCL);

	//DPORT &= ~( _BV( DSDA ) | _BV(DSCL) );
	//DDDR  &= ~( _BV( DSDA ) | _BV(DSCL) );
}

I2CSTATICODE void I2CFNCOLLAPSE( I2CPREFIX, SendStart )()
{
	//DDDR  |= _BV( DSDA );
	PIN_DIR_INPUT = _BV(DSCL);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	PIN_DIR_OUTPUT = _BV(DSDA);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	//DDDR  |= _BV( DSCL );
	PIN_DIR_OUTPUT = _BV(DSCL);
}

I2CSTATICODE void I2CFNCOLLAPSE( I2CPREFIX, SendStop ) ()
{
	//DDDR &= ~_BV( DSCL );
	PIN_DIR_OUTPUT = _BV(DSDA);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	PIN_DIR_INPUT = _BV(DSCL);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	//DDDR &= ~_BV( DSDA );
	PIN_DIR_INPUT = _BV(DSDA);

	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
}

//Return nonzero on failure.
I2CSTATICODE unsigned char I2CFNCOLLAPSE( I2CPREFIX, SendByte )( unsigned char data )
{
	unsigned int i;
	//Assume we are in a started state (DSCL = 0 & DSDA = 0)
/* DO NOT USE
	DPORT |= _BV(DSDA);
	DDDR |= _BV(DSDA);
*/
	for( i = 0; i < 8; i++ )
	{
		I2CDELAY_FUNC( 1 * I2CSPEEDBASE );

		if( data & 0x80 )
			/*DDDR &= ~_BV( DSDA );*/PIN_DIR_INPUT = _BV(DSDA);
		else
			/*DDDR |= _BV( DSDA );*/PIN_DIR_OUTPUT = _BV(DSDA);

		data<<=1;

		I2CDELAY_FUNC( 1 * I2CSPEEDBASE );

		/*DDDR &= ~_BV( DSCL );*/PIN_DIR_INPUT = _BV(DSCL);

		I2CDELAY_FUNC( 2 * I2CSPEEDBASE );

		/*DDDR |= _BV( DSCL );*/PIN_DIR_OUTPUT = _BV(DSCL);
	}

	//Immediately after sending last bit, open up DDDR for control.
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	/*DDDR &= ~_BV( DSDA );*/ PIN_DIR_INPUT = _BV(DSDA);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	/*DDDR &= ~_BV( DSCL );*/PIN_DIR_INPUT = _BV(DSCL);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	i = PIN_IN & _BV( DSDA );
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	/*DDDR &= ~_BV( DSCL );*/PIN_DIR_OUTPUT = _BV(DSCL);
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	///*DDDR &= ~_BV( DSDA );*/ PIN_DIR_OUTPUT = _BV(DSDA);
	//I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	return !!i;
}

#if I2CNEEDGETBYTE

I2CSTATICODE unsigned char I2CFNCOLLAPSE( I2CPREFIX, GetByte )( uint8_t send_nak )
{
	unsigned char i;
	unsigned char ret = 0;

	/*DDDR &= ~_BV( DSDA );*/PIN_DIR_INPUT = _BV(DSDA);

	for( i = 0; i < 8; i++ )
	{
		I2CDELAY_FUNC( 1 * I2CSPEEDBASE );

		/*DDDR &= ~_BV( DSCL );*/PIN_DIR_INPUT = _BV(DSCL);

		I2CDELAY_FUNC( 2 * I2CSPEEDBASE );

		ret<<=1;

		if( PIN_IN & _BV( DSDA ) )
			ret |= 1;

		/*DDDR |= _BV( DSCL );*/PIN_DIR_OUTPUT = _BV(DSCL);
	}

	//Send ack.
	if( send_nak )
	{
	}
	else
	{
		/*DDDR |= _BV( DSDA );*/PIN_DIR_OUTPUT = _BV(DSDA);
	}

	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	/*DDDR &= ~_BV( DSCL );*/PIN_DIR_INPUT = _BV(DSCL);
	I2CDELAY_FUNC( 3 * I2CSPEEDBASE );
	/*DDDR |= _BV( DSCL );*/PIN_DIR_OUTPUT = _BV(DSCL);

	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	/*DDDR &= ~_BV( DSDA );*/PIN_DIR_INPUT = _BV(DSDA);

	return ret;
}

#endif
