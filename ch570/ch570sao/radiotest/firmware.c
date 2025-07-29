#include "ch32fun.h"
#include <stdio.h>

#define ISLER_CALLBACK iSLERCallback
static void iSLERCallback();
#include "iSLER.h"


#define LL_TX_POWER_0_DBM 0x12
#define PHY_MODE          PHY_1M

#define REPORT_ALL 1

uint8_t frame_info[] = {0xff, 0x10}; // PDU, len


uint8_t adv[] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
				 0x03, 0x19, 0x00, 0x00, // 0x19: "Appearance", 0x00, 0x00: "Unknown"
				 0x06, 0x09, 'R', 'X', ':', '?', '?'}; // 0x09: "Complete Local Name"
uint8_t adv_channels[] = {37,38,39};
uint8_t hex_lut[] = "0123456789ABCDEF";

int rxFrames;

void iSLERCallback()
{
	Frame_RX(frame_info, 37, PHY_MODE);
	uint8_t *frame = (uint8_t*)LLE_BUF;
	//printf("%3d %d\n", frame[0], frame[1]);
	rxFrames++;
}
#if 0

void incoming_frame_handler() {
	uint8_t *frame = (uint8_t*)LLE_BUF;
#if 0
	printf("RSSI:%d len:%d MAC:", frame[0], frame[1]);
	for(int i = 7; i > 2; i--) {
		printf("%02x:", frame[i]);
	}
	printf("%02x data:", frame[2]);
	for(int i = 8; i < frame[1] +2; i++) {
		printf("%02x ", frame[i]);
	}
	printf("\n");
#endif
	rxFrames++;
	// advertise reception of a FindMy frame
	if(REPORT_ALL || (frame[8] == 0x1e && frame[10] == 0x4c)) {
		adv[sizeof(adv) -2] = hex_lut[(frame[7] >> 4)];
		adv[sizeof(adv) -1] = hex_lut[(frame[7] & 0xf)];
		for(int c = 0; c < sizeof(adv_channels); c++) {
			Frame_TX(adv, sizeof(adv), adv_channels[c], PHY_MODE);
		}
	}
}
#endif

int main()
{
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx

//	printf( "Startup\n" );

	RFCoreInit(LL_TX_POWER_0_DBM);

	for(int c = 0; c < sizeof(adv_channels); c++) {
		Frame_TX(adv, sizeof(adv), adv_channels[c], PHY_MODE);
	}

	Frame_RX(frame_info, 37, PHY_MODE);

	while(1) {
		//while(!rx_ready);
		Delay_Ms(1000);

		Frame_TX(adv, sizeof(adv), 37, PHY_MODE);
		Frame_RX(frame_info, 37, PHY_MODE);

		//incoming_frame_handler();

		int f = rxFrames; rxFrames = 0;
		printf( "%d\n", f );

//PFIC->SCTLR = 0x0e; // Sleep, Deep, WFE
//__ASM volatile ("wfi");
//__ASM volatile ("wfi");

	}
}
