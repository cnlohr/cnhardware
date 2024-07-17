#ifndef _CH32V307GIGABIT_H
#define _CH32V307GIGABIT_H

static int ch32v307ethInit()
{
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
		(8<<RCC_PLL3MUL_OFFSET) | // PLL3 = x10
		(1<<RCC_PREDIV2_OFFSET) | // PREDIV = /2; Prediv Freq = 12.5MHz.
		(1<<RCC_ETH1GSRC_OFFSET)| // Use PLL3 for RGMII
		0;
	RCC->CTLR |= RCC_PLL3ON;
	int timeout;

	for( timeout = 10000; timeout > 0; timeout--) if (RCC->CTLR & RCC_PLL3RDY) break;
	if( timeout == 0 ) return -5;

	RCC->CFGR2 |= RCC_ETH1G_125M_EN;
	funPinMode( PA8, GPIO_CFGLR_OUT_50Mhz_AF_PP );
}

#endif

