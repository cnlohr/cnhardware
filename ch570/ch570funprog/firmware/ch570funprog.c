#include <stdio.h>
#include "ch32fun.h"
#include "fsusb.h"

#define POWER_IO    PA7
#define SWC_IO      PA3
#define SWD_IO      PA2
#define USB_DATA_BUF_SIZE 256

__attribute__((aligned(4))) static volatile uint8_t gs_usb_data_buf[USB_DATA_BUF_SIZE];

/*
__HIGH_CODE
void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
		funDigitalWrite( LED, FUN_LOW ); // Turn on LED
		Delay_Ms(33);
		funDigitalWrite( LED, FUN_HIGH ); // Turn off LED
		if(i) Delay_Ms(33);
	}
}
*/

int HandleSetupCustom( struct _USBState * ctx, int setup_code) {
	return 0;
}

int HandleInRequest( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	return 0;
}

__HIGH_CODE
void HandleDataOut( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	// this is actually the data rx handler
	if ( endp == 0 ) {
		// this is in the hsusb.c default handler
		ctx->USBFS_SetupReqLen = 0; // To ACK
	}
	else if( endp == USB_EP_RX ) {
		if(len == 4 && ((uint32_t*)data)[0] == 0x010001a2) {
			USBFSReset();
			//blink(2);
			jump_isprom();
		}
		else if( gs_usb_data_buf[0] == 0 ) {
			gs_usb_data_buf[0] = len;
			for(int i = 0; i < len; i++) {
				gs_usb_data_buf[i +1] = data[i];
			}
		}
		else {
			// previous usb buffer not consumed yet, what should we do?
		}

		ctx->USBFS_Endp_Busy[USB_EP_RX] = 0;
	}
}

void Tick()
{
	//funDigitalWrite( SWD_IO, FUN_HIGH ); // Turn off LED
	//funDigitalWrite( SWD_IO, FUN_LOW ); // Turn on LED
}

__HIGH_CODE
void ASetup()
{
	SYS_SAFE_ACCESS( R8_PWM_CONFIG = (2 << 1) | 0xff; ); // 64-cycle
	SYS_SAFE_ACCESS( R8_SLP_CLK_OFF1=2; ); // disable AES
	SYS_SAFE_ACCESS( R8_SLP_CLK_OFF0=(1<<4); ); // disable uart
	SYS_SAFE_ACCESS( R16_PWM_CLOCK_DIV = 4; );

	SYS_SAFE_ACCESS( R8_PWM_POLAR = RB_PWM3_POLAR; );
	//R8_PWM_CONFIG = 0b100 | RB_PWM_CYCLE_SEL;

	*((volatile uint16_t *)((&R8_PWM2_DATA))) = 12;
	*((volatile uint16_t *)((&R8_PWM3_DATA))) = 12;

	*((volatile uint16_t *)((&R16_PWM2_DATA))) = 12;
	*((volatile uint16_t *)((&R16_PWM3_DATA))) = 12;

	R8_PWM_OUT_EN = RB_PWM2_OUT_EN | RB_PWM3_OUT_EN;
	R8_PWM_CONFIG |= RB_PWM_SYNC_EN;
	R8_PWM_CONFIG |= RB_PWM_SYNC_START;

}

int main() {
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

	//HSECFG_Capacitance(HSECap_10p);

//	funPinMode( SWC_IO, GPIO_CFGLR_OUT_10Mhz_PP ); // GPIO_CFGLR_IN_PU
//	funPinMode( SWD_IO, GPIO_CFGLR_OUT_10Mhz_PP );

//	USBFSSetup();

//	R32_PA_PD_DRV = SWD_IO | SWC_IO; // 20mA
	R32_PA_DIR    = SWC_IO | SWD_IO | POWER_IO; // Drive out.

//	R16_PIN_ALTERNATE_H = 1; // Map UART off of port.

	ASetup();
	// Remap
printf( "%04x %04x %04x %04x %04x\n", R8_PWM_OUT_EN, R8_PWM_CONFIG, R16_PWM_CLOCK_DIV, R8_PWM3_DATA, R8_SLP_CLK_OFF1 );

	funDigitalWrite( POWER_IO, FUN_LOW ); // Turn on output.
while(1) {
	//funDigitalWrite( SWD_IO, FUN_LOW );
	//funDigitalWrite( SWD_IO, FUN_HIGH );
}

	while(1) {

		Tick();

		if( gs_usb_data_buf[0] ) {
			// Send data back to PC.
			while( USBFSCTX.USBFS_Endp_Busy[USB_EP_TX] & 1 );
			USBFS_SendEndpointNEW( USB_EP_TX, (uint8_t*)&gs_usb_data_buf[1], gs_usb_data_buf[0], /*copy=*/1 ); // USBFS needs a copy here
			gs_usb_data_buf[0] = 0;
		}
	}
}
