#ifndef _CH32V307GIGABIT_H
#define _CH32V307GIGABIT_H

// #define CH32V307GIGABIT_MCO25 1
// #define CH32V307GIGABIT_PHYADDRESS 0

#ifndef CH32V307GIGABIT_MCO25
#define CH32V307GIGABIT_MCO25 1
#endif

#ifndef CH32V307GIGABIT_PHYADDRESS
#define CH32V307GIGABIT_PHYADDRESS 0
#endif

#ifndef CH32V307GIGABIT_PHY_TIMEOUT
#define CH32V307GIGABIT_PHY_TIMEOUT 0x10000
#endif

static int ch32v307ethPHYRegWrite(uint32_t reg, uint32_t val);
static int ch32v307ethPHYRegRead(uint32_t reg);
static void ch32v307ethGetMacInUC( uint8_t * mac );
static int ch32v307ethInit( );

// Based on ETH_WritePHYRegister
static int ch32v307ethPHYRegWrite( uint32_t reg, uint32_t val )
{
	ETH->MACMIIDR = val;
	ETH->MACMIIAR = ETH_MACMIIAR_CR_Div42 /* = 0, per 27.1.8.1.4 */ |
		(((uint32_t)CH32V307GIGABIT_PHYADDRESS << 11)) | // ETH_MACMIIAR_PA
		(((uint32_t)reg << 6) & ETH_MACMIIAR_MR) |
		ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;

	uint32_t	timeout = 0x100000;
	while( ( ETH->MACMIIAR & ETH_MACMIIAR_MB ) && --timeout );

	// If timeout = 0, is an error.
	return timeout ? 0 : -1;
}

static int ch32v307ethPHYRegRead( uint32_t reg )
{
	ETH->MACMIIAR = ETH_MACMIIAR_CR_Div42 /* = 0, per 27.1.8.1.4 */ |
		((uint32_t)CH32V307GIGABIT_PHYADDRESS << 11) | // ETH_MACMIIAR_PA
		(((uint32_t)reg << 6) & ETH_MACMIIAR_MR) |
		(0 /*!ETH_MACMIIAR_MW*/) | ETH_MACMIIAR_MB;

	uint32_t	timeout = 0x100000;
	while( ( ETH->MACMIIAR & ETH_MACMIIAR_MB ) && --timeout );

	// If timeout = 0, is an error.
	return timeout ? ETH->MACMIIDR : -1;
}

static void ch32v307ethGetMacInUC( uint8_t * mac )
{
	// Mac is backwards.
	const uint8_t *macaddr = (const uint8_t *)(ROM_CFG_USERADR_ID+5);
	for( int i = 0; i < 6; i++ )
	{
		mac[i] = *(macaddr--);
	}
}

static int ch32v307ethInit()
{
	funPinMode( PA10, GPIO_CFGLR_OUT_50Mhz_PP ); //PHY_RSTB (For reset)
	funDigitalWrite( PA10, FUN_LOW );

	// Pull-up MDIO

	funPinMode( PD9, GPIO_CFGLR_OUT_50Mhz_PP ); //Pull-up control (DO NOT CHECK IN, ADD RESISTOR)
	funDigitalWrite( PD9, FUN_HIGH );

	funPinMode( PB13, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDIO
	funPinMode( PB12, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDC

	// Will be required later.
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO;

	// https://cnlohr.github.io/microclockoptimizer/?chipSelect=ch32vx05_7%2Cd8c&HSI=1,8&HSE=0,8&PREDIV2=1,1&PLL2CLK=1,7&PLL2VCO=0,72&PLL3CLK=1,1&PLL3VCO=0,100&PREDIV1SRC=1,0&PREDIV1=1,2&PLLSRC=1,0&PLL=0,4&PLLVCO=1,144&SYSCLK=1,2&
	// Clock Tree:
	// 8MHz Input
	// PREDIV2 = 2 (1 in register) = 4MHz
	// PLL2 = 9 (7 in register) = 36MHz / PLL2VCO = 72MHz
	// PLL3CLK = 12.5 (1 in register) = 50MHz = 100MHz VCO
	// PREDIV1SRC = HSE (1 in register) = 8MHz
	// PREDIV1 = 2 (1 in register).
	// PLLSRC = PREDIV1 (0 in register) = 4MHz
	// PLL = 18 (0 in register) = 72MHz 
	// PLLVCO = 144MHz 
	// SYSCLK = PLLVCO = 144MHz
	// Use EXT_125M (ETH1G_SRC)

	// Switch processor back to HSI so we don't eat dirt.
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSI;

	// Setup clock tree.
	RCC->CFGR2 |= 
		(1<<RCC_PREDIV2_OFFSET) | // PREDIV = /2; Prediv Freq = 4MHz
		(1<<RCC_PLL3MUL_OFFSET) | // PLL3 = x12.5 (PLL3 = 50MHz)
		(2<<RCC_ETH1GSRC_OFFSET)| // External source for RGMII
		(7<<RCC_PLL2MUL_OFFSET) | // PLL2 = x9 (PLL2 = 36MHz)
		(1<<RCC_PREDIV1_OFFSET) | // PREDIV1 = /2; Prediv freq = 50MHz
		0;

	// Power on PLLs
	RCC->CTLR |= RCC_PLL3ON | RCC_PLL2ON;
	int timeout;

	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLL3RDY) break;
	if( timeout == 0 ) return -5;
	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLL2RDY) break;
	if( timeout == 0 ) return -6;

	// PLL = x18 (0 in register)
	RCC->CFGR0 = ( RCC->CFGR0 & ~(0xf<<18)) | 0;
	RCC->CTLR |= RCC_PLLON;

	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLLRDY) break;
	if( timeout == 0 ) return -7;

	// Switch to PLL.
#ifdef CH32V307GIGABIT_MCO25
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL | (9<<24); // And output clock on PA8
#else
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL;
#endif

	// For clock in.
	funPinMode( PB1, GPIO_CFGLR_IN_FLOAT ); //GMII_CLK125

	RCC->CFGR2 |= RCC_ETH1G_125M_EN; // Enable 125MHz clock.

	// Use RGMII
	EXTEN->EXTEN_CTR |= (1<<3); //EXTEN_ETH_RGMII_SEL;

	// For clock output to Ethernet module.
	funPinMode( PA8, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // PHY_CKTAL

	funPinMode( PB0, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD3
	funPinMode( PC5, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD2
	funPinMode( PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD1
	funPinMode( PA7, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD0
	funPinMode( PA3, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXCTL
	funPinMode( PA2, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXC
	funPinMode( PA1, GPIO_CFGLR_IN_FLOAT ); // GMII_RXD3
	funPinMode( PA0, GPIO_CFGLR_IN_FLOAT ); // GMII_RXD2
	funPinMode( PC3, GPIO_CFGLR_IN_FLOAT ); // GMII_RXD1
	funPinMode( PC2, GPIO_CFGLR_IN_FLOAT ); // GMII_RXD0
	funPinMode( PC1, GPIO_CFGLR_IN_FLOAT ); // GMII_RXCTL
	funPinMode( PC0, GPIO_CFGLR_IN_FLOAT ); // GMII_RXC

	// Power on and reset.
	RCC->AHBPCENR |= RCC_ETHMACEN | RCC_ETHMACTXEN | RCC_ETHMACRXEN;
	RCC->AHBRSTR |= RCC_ETHMACRST;
	RCC->AHBRSTR &=~RCC_ETHMACRST;

	// Register setup.

	// Configure MDC/MDIO
    ETH->MACMIIAR = 0;

	// Update MACCR
	ETH->MACCR =
		( 0 << 29 ) | // No clock delay
		( 0 << 23 ) | // Max RX = 2kB
		( 0 << 22 ) | // Max TX = 2kB 
		( 0 << 21 ) | // Rated Drive
		( 1 << 20 ) | // Bizarre re-use of termination resistor terminology? TODO: Examine me.
		( 0 << 17 ) | // IFG = 0, 96-bit guard time.
		( 2 << 14 ) | // FES = 2 = 1 GBit/s
		( 0 << 12 ) | // Self Loop = 0
		( 1 << 11 ) | // Full-Duplex Mode
		( 1 << 10 ) | // IPCO = 1, Check TCP, UDP, ICMP header checksums.
		( 1 << 7  ) | // APCS (automatically strip frames)
		( 1 << 3  ) | // TE (Transmit enable!)
		( 1 << 2  ) | // RE (Receive Enable)
		( 0 << 1  ) | // TCF = 0 (Potentailly change if clocking is wrong)
		0;

	ETH->MACFFR =
		( 1 << 31 ) | // RA = Pass all frames.
		( 0 << 0  ) | // PM = Promiscuous mode = 0;
		0;

	ETH->MACFCR = 0; // No pause frames.

	ETH->MACVLANTR = 0; // No VLAN tagging.

	ETH->DMAOMR = 0; // Default DMA Forward Behavior

	// Release PHY from reset.
	funDigitalWrite( PA10, FUN_HIGH );

	Delay_Ms(5); 	// Inconsistent at 3.5ms

	// Reset the physical layer
	ch32v307ethPHYRegWrite( PHY_BCR, PHY_Reset );

	Delay_Ms(10);    // Consistent at ~9ms

	ch32v307ethPHYRegWrite( PHY_BCR, 
		1<<12 | // Auto negotiate
		1<<8 | // Duplex
		0 );

	Delay_Ms(10);    // Consistent at ~9ms.

/*
    ch32v307ethPHYRegWrite( 0x1F, 0x0a43 );
	// Need to sometimes double-read according to WCH.
	Delay_Ms(10);    // Consistent at ~9ms.
    ch32v307ethPHYRegRead( 0x1A);
	Delay_Ms(10);    // Consistent at ~9ms.
	int phy_stat = ch32v307ethPHYRegRead( 0x1A );
*/
	int phy_stat = 0;

while( 1 )
{
	Delay_Ms(100);    // Consistent at ~9ms.

	ch32v307ethPHYRegRead( 0x11 );
	Delay_Ms(10);    // Consistent at ~9ms.

	int gbsr = ch32v307ethPHYRegRead( 0x11 );
	
	// Next step: Start talking over MDIO.
	printf( "%04x, %04x\n", gbsr, phy_stat );
}
	uint8_t mac[6] = { 0 };
	ch32v307ethGetMacInUC( mac );
	printf( "%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

	return 0;
}

#endif

