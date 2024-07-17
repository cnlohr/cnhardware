#ifndef _CH32V307GIGABIT_H
#define _CH32V307GIGABIT_H

static int ch32v307ethInit()
{
	funPinMode( PA10, GPIO_CFGLR_OUT_50Mhz_PP ); //PHY_RSTB (For reset)
	funDigitalWrite( PA10, FUN_LOW );

	RCC->AHBPCENR |= RCC_ETHMACEN | RCC_ETHMACTXEN | RCC_ETHMACRXEN;
	RCC->AHBRSTR |= RCC_ETHMACRST;
	RCC->AHBRSTR &=~RCC_ETHMACRST;

	// Clock Tree:
	// 25MHz Input
	// Prediv = /4
	// Prediv = 6.25MHz
	//   PLL3 = *20 -> 125MHz
	//   PLL2 = *8
	// enable 125MHz clock out, on PA8
	// Use PLL3 for RGMII clock
	RCC->CFGR2 |= 
		(1<<RCC_PREDIV2_OFFSET) | // PREDIV = /2; Prediv Freq = 12.5MHz.
		(8<<RCC_PLL3MUL_OFFSET) | // PLL3 = x10 (PLL3 = 125MHz)
		(1<<RCC_ETH1GSRC_OFFSET)| // Use PLL3 for RGMII
		(6<<RCC_PLL2MUL_OFFSET) | // PLL2 = x8 (PLL2 = 100MHz)
		(1<<RCC_PREDIV1_OFFSET) | // PREDIV1 = /2; Prediv freq = 50MHz
		0;
	RCC->CTLR |= RCC_PLL3ON | RCC_PLL2ON;
	int timeout;

	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLL3RDY) break;
	if( timeout == 0 ) return -5;
	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLL2RDY) break;
	if( timeout == 0 ) return -6;

	// PLL = x3 (150MHz)
	RCC->CFGR0 = ( RCC->CFGR0 & ~(0xf<<18)) | RCC_PLLMULL3;
	RCC->CTLR |= RCC_PLLON;

	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLLRDY) break;
	if( timeout == 0 ) return -7;

	// Switch to PLL.
	RCC->CFGR0 = ( RCC->CFGR0 & ~(0x3)) | RCC_SW_PLL;

	RCC->CFGR2 |= RCC_ETH1G_125M_EN;

	// For clock in
	funPinMode( PA8, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // PHY_CKTAL
	funPinMode( PB0, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD3
	funPinMode( PC5, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD2
	funPinMode( PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD1
	funPinMode( PA7, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXD0
	funPinMode( PA3, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXCTL
	funPinMode( PA2, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_TXC
	funPinMode( PA1, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXD3
	funPinMode( PA0, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXD2
	funPinMode( PC3, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXD1
	funPinMode( PC2, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXD0
	funPinMode( PC1, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXCTL
	funPinMode( PC0, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // GMII_RXC

	funPinMode( PA13, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDIO
	funPinMode( PA12, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDC

	// For clock out.
	funPinMode( PB1, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_CLK125

	// Release from reset.
	funDigitalWrite( PA10, FUN_HIGH );


	return 0;
}

#endif

