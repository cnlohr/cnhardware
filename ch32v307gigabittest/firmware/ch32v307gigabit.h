#ifndef _CH32V307GIGABIT_H
#define _CH32V307GIGABIT_H

/* This file is written against the RTL8211E

  TODO:
	1. Add interrupt features to PHY (see PHY_InterruptInit)


*/

// #define CH32V307GIGABIT_MCO25 1
// #define CH32V307GIGABIT_PHYADDRESS 0

#define CH32V307GIGABIT_RXBUFNB 8
#define CH32V307GIGABIT_TXBUFNB 8
#define CH32V307GIGABIT_BUFFSIZE 1520

#define CH32V307GIGABIT_CFG_CLOCK_DELAY 4
#define CH32V307GIGABIT_CFG_CLOCK_PHASE 0

#ifndef CH32V307GIGABIT_MCO25
#define CH32V307GIGABIT_MCO25 1
#endif

#ifndef CH32V307GIGABIT_PHYADDRESS
#define CH32V307GIGABIT_PHYADDRESS 0
#endif

#ifndef CH32V307GIGABIT_PHY_TIMEOUT
#define CH32V307GIGABIT_PHY_TIMEOUT 0x10000
#endif

#ifndef CH32V307GIGABIT_PHY_RSTB
#define CH32V307GIGABIT_PHY_RSTB PA10
#endif


// Additional definitions, not part of ch32v003fun.h

// ETH DMA structure definition (From ch32v30x_eth.c
typedef struct
{
  uint32_t   volatile Status;       /* Status */
  uint32_t   ControlBufferSize;     /* Control and Buffer1, Buffer2 lengths */
  uint32_t   Buffer1Addr;           /* Buffer1 address pointer */
  uint32_t   Buffer2NextDescAddr;   /* Buffer2 or next descriptor address pointer */
} ETH_DMADESCTypeDef;


// Data pursuent to ethernet.
uint8_t ch32v307eth_mac[6] = { 0 };
ETH_DMADESCTypeDef ch32v307eth_DMARxDscrTab[CH32V307GIGABIT_RXBUFNB] __attribute__((aligned(4)));            // MAC receive descriptor, 4-byte aligned
ETH_DMADESCTypeDef ch32v307eth_DMATxDscrTab[CH32V307GIGABIT_TXBUFNB] __attribute__((aligned(4)));            // MAC send descriptor, 4-byte aligned
uint8_t  ch32v307eth_MACRxBuf[CH32V307GIGABIT_RXBUFNB*CH32V307GIGABIT_BUFFSIZE] __attribute__((aligned(4))); // MAC receive buffer, 4-byte aligned
uint8_t  ch32v307eth_MACTxBuf[CH32V307GIGABIT_TXBUFNB*CH32V307GIGABIT_BUFFSIZE] __attribute__((aligned(4))); // MAC send buffer, 4-byte aligned
ETH_DMADESCTypeDef * pDMARxSet;
ETH_DMADESCTypeDef * pDMATxSet;

static int ch32v307ethPHYRegWrite(uint32_t reg, uint32_t val);
static int ch32v307ethPHYRegRead(uint32_t reg);
static void ch32v307ethGetMacInUC( uint8_t * mac );
static int ch32v307ethInit( );
static void PrintCurrentPHYState();




static void PrintCurrentPHYState()
{
	ch32v307ethPHYRegRead( 0x11 );
	int test = ch32v307ethPHYRegRead( 0x11 );
	int speed = ((test>>14)&3);
	printf( "%02x:%02x:%02x:%02x:%02x:%02x\n", ch32v307eth_mac[0], ch32v307eth_mac[1], ch32v307eth_mac[2], ch32v307eth_mac[3], ch32v307eth_mac[4], ch32v307eth_mac[5] );
	printf( "%s %dMBPs Duplex:%d PR:%d SDR:%d %s PLOK:%d JAB:%d\n",
		((test>>10)&1)?"LINK OK":"NO LINK", (speed<2)?(speed==0)?10:100:(speed==2)?1000:0, (test>>13)&1, (test>>12)&1, (test>>11)&1,
		((test>>6)&1)?"Crossover":"Straight", (test>>1)&1, test&1 );
}


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
	int i;

	funPinMode( CH32V307GIGABIT_PHY_RSTB, GPIO_CFGLR_OUT_50Mhz_PP ); //PHY_RSTB (For reset)
	funDigitalWrite( CH32V307GIGABIT_PHY_RSTB, FUN_LOW );

	// Configure strapping.
	funPinMode( PA1, GPIO_CFGLR_IN_PUPD ); // GMII_RXD3
	funPinMode( PA0, GPIO_CFGLR_IN_PUPD ); // GMII_RXD2
	funPinMode( PC3, GPIO_CFGLR_IN_PUPD ); // GMII_RXD1
	funPinMode( PC2, GPIO_CFGLR_IN_PUPD ); // GMII_RXD0
	funDigitalWrite( PA1, FUN_HIGH );
	funDigitalWrite( PA0, FUN_HIGH );
	funDigitalWrite( PC3, FUN_HIGH ); // No TX Delay
	funDigitalWrite( PC2, FUN_HIGH );

	// Pull-up MDIO
	funPinMode( PD9, GPIO_CFGLR_OUT_50Mhz_PP ); //Pull-up control (DO NOT CHECK IN, ADD RESISTOR)
	funDigitalWrite( PD9, FUN_HIGH );

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

	ETH->DMABMR |= ETH_DMABMR_SR;

	// Wait for reset to complete.
	for( timeout = 10000; timeout > 0 && (ETH->DMABMR & ETH_DMABMR_SR); timeout-- )
	{
		Delay_Us(10);
	}

	// Use RGMII
	EXTEN->EXTEN_CTR |= (1<<3); //EXTEN_ETH_RGMII_SEL;

	funPinMode( PB13, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDIO
	funPinMode( PB12, GPIO_CFGLR_OUT_50Mhz_AF_PP ); //GMII_MDC

	// For clock output to Ethernet module.
	funPinMode( PA8, GPIO_CFGLR_OUT_50Mhz_AF_PP ); // PHY_CKTAL

	// Release PHY from reset.
	funDigitalWrite( CH32V307GIGABIT_PHY_RSTB, FUN_HIGH );

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
		( CH32V307GIGABIT_CFG_CLOCK_DELAY << 29 ) | // No clock delay
		( 0 << 23 ) | // Max RX = 2kB
		( 0 << 22 ) | // Max TX = 2kB 
		( 0 << 21 ) | // Rated Drive (instead of energy savings mode) (10M PHY only)
		( 1 << 20 ) | // Bizarre re-use of termination resistor terminology? (10M PHY Only)
		( 0 << 17 ) | // IFG = 0, 96-bit guard time.
		( 2 << 14 ) | // FES = 2 = 1 GBit/s
		( 0 << 12 ) | // Self Loop = 0
		( 1 << 11 ) | // Full-Duplex Mode
		( 1 << 10 ) | // IPCO = 1, Check TCP, UDP, ICMP header checksums.
		( 1 << 7  ) | // APCS (automatically strip frames)
		( 1 << 3  ) | // TE (Transmit enable!)
		( 1 << 2  ) | // RE (Receive Enable)
		( CH32V307GIGABIT_CFG_CLOCK_PHASE << 1  ) | // TCF = 0 (Potentailly change if clocking is wrong)
		0;

	ETH->MACFFR =
		( 1 << 31 ) | // RA = Pass all frames.
		( 0 << 0  ) | // PM = Promiscuous mode = 0;
		0;

	ETH->MACFCR = 0; // No pause frames.

	ETH->MACVLANTR = 0; // No VLAN tagging.

	ETH->DMAOMR = 0; // Default DMA Forward Behavior

	Delay_Ms(5); 	// Inconsistent at 3.5ms

	// Reset the physical layer
	ch32v307ethPHYRegWrite( PHY_BCR,
		PHY_Reset |
		1<<12 | // Auto negotiate
		1<<8 | // Duplex
		1<<6 | // Speed Bit.
		0 );

	// De-assert reset.
	ch32v307ethPHYRegWrite( PHY_BCR,
		1<<12 | // Auto negotiate
		1<<8 | // Duplex
		1<<6 | // Speed Bit.
		0 );

	// TODO: Here is where we would want to configure the PHY to trigger interrupts on link changes.

	ch32v307ethGetMacInUC( ch32v307eth_mac );

	ETH->MACA0HR = (uint32_t)((ch32v307eth_mac[5]<<8) | ch32v307eth_mac[4]);
	ETH->MACA0LR = (uint32_t)(ch32v307eth_mac[0] | (ch32v307eth_mac[1]<<8) | (ch32v307eth_mac[2]<<16) | (ch32v307eth_mac[3]<<24));

	// Configure RX/TX chains.
    ETH_DMADESCTypeDef *tdesc;
	for(i = 0; i < CH32V307GIGABIT_TXBUFNB; i++)
	{
		tdesc = ch32v307eth_DMATxDscrTab + i;
		tdesc->ControlBufferSize = 0;
		tdesc->Status = ETH_DMATxDesc_TCH | ETH_DMATxDesc_IC;
		tdesc->Buffer1Addr = (uint32_t)(&ch32v307eth_MACTxBuf[i * CH32V307GIGABIT_BUFFSIZE]);
		tdesc->Buffer2NextDescAddr = (i < CH32V307GIGABIT_TXBUFNB - 1) ? ((uint32_t)(ch32v307eth_DMATxDscrTab + i + 1)) : (uint32_t)ch32v307eth_DMATxDscrTab;
	}
    ETH->DMATDLAR = (uint32_t)ch32v307eth_DMATxDscrTab;
	for(i = 0; i < CH32V307GIGABIT_RXBUFNB; i++)
	{
		tdesc = ch32v307eth_DMARxDscrTab + i;
		tdesc->Status = ETH_DMARxDesc_OWN;
		tdesc->ControlBufferSize = ETH_DMARxDesc_RCH | (uint32_t)CH32V307GIGABIT_BUFFSIZE;
		tdesc->Buffer1Addr = (uint32_t)(&ch32v307eth_MACRxBuf[i * CH32V307GIGABIT_BUFFSIZE]);
		tdesc->Buffer2NextDescAddr = (i < CH32V307GIGABIT_RXBUFNB - 1) ? (uint32_t)(ch32v307eth_DMARxDscrTab + i + 1) : (uint32_t)(ch32v307eth_DMARxDscrTab);
	}
	ETH->DMARDLAR = (uint32_t)ch32v307eth_DMARxDscrTab;
	pDMARxSet = ch32v307eth_DMARxDscrTab;
	pDMATxSet = ch32v307eth_DMATxDscrTab;

	// Transmit good frame half interrupt mask.
	ETH->MMCTIMR = ETH_MMCTIMR_TGFM;

	// Receive a good frame half interrupt mask.
	// Receive CRC error frame half interrupt mask.
	ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

	ETH->DMAIER = ETH_DMA_IT_NIS | // Normal interrupt enable.
				ETH_DMA_IT_R |     // Receive
				ETH_DMA_IT_T |     // Transmit
				ETH_DMA_IT_AIS |   // Abnormal interrupt 
				ETH_DMA_IT_RBU;    // Receive buffer unavailable interrupt enable

	NVIC_EnableIRQ( ETH_IRQn );


	return 0;
}

void ETH_IRQHandler( void ) __attribute__((interrupt));
void ETH_IRQHandler( void )
{
    uint32_t int_sta;
printf( "IRQ\n" );
    int_sta = ETH->DMASR;
    if (int_sta & ETH_DMA_IT_AIS)
    {
		// Receive buffer unavailable interrupt enable.
        if (int_sta & ETH_DMA_IT_RBU)
        {
            ETH->DMASR = ETH_DMA_IT_RBU;
            if((CHIP_ID & 0xf0) == 0x10)
            {
                ((ETH_DMADESCTypeDef *)(((ETH_DMADESCTypeDef *)(ETH->DMACHRDR))->Buffer2NextDescAddr))->Status = ETH_DMARxDesc_OWN;

                /* Resume DMA reception */
                ETH->DMARPDR = 0;
            }
        }
        ETH->DMASR = ETH_DMA_IT_AIS;
    }

    if( int_sta & ETH_DMA_IT_NIS )
    {
        if( int_sta & ETH_DMA_IT_R )
        {
            /*If you don't use the Ethernet library,
             * you can do some data processing operations here*/
			printf( "GOT PACKET\n" );
            ETH->DMASR = ETH_DMA_IT_R;
        }
        if( int_sta & ETH_DMA_IT_T )
        {
            ETH->DMASR = ETH_DMA_IT_T;
        }
        ETH->DMASR = ETH_DMA_IT_NIS;
    }
}


#endif

