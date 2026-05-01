
# opampandadcfromled

Using a 1W LED with D+ to PC0, and D- to PA1 as an LED or a photodiode with a ch32v006.

You can turn it back into an LED by driving the pins the other way.

It operates with the built-in op-amp as a TIA.


# Options:

## From-Zero

// PC0 = vdd = D-
// PA2 = D+, pull-down
// VBEN = 0
// nsel = 110
// pgadef = 0

very sensitive but has dead zone that's pretty rough.  needs led less than 1' away


## From-High


// New Attempt:
// PC0 = GND = D+
// PA2 = Pull-Up = D-
// PA4 = VDD
// VBEN = 0
// nsel = 110
// pgadef = 1

Kinda noisy.  But no dead zone.  Measuring voltages near VCC is rough.

## From-zero another attempt

// Use OPA_CTLR1_VBEN/VBSEL to bias.

ACTUALLY THIS WORKS PRETTY GOOD.

// New Attempt:
// PC0 = GND = D+
// PA2 = Pull-Up = D-
// PA4 = unused
// VBEN = 0
// nsel = 011
// pgadef = 1

	OPA->CTLR1 =
		OPA_CTLR1_VBEN | // VBEN to pull + to VDD vs VBEN = 0 -> Directly connect + to that pin.
		OPA_CTLR1_VBSEL |
		0 | // Connect PSEL1 to PA2
		0 | // Disconnect PSEL1.
		(0b011<<8) | // NSEL some gain
		//OPA_CTLR1_PGADIF | // Connect PGADIF to PA4 (now VCC) vs GND (0) (If commented)
		OPA_CTLR1_FB_EN1 | // FB_EN1 = 1 (Enable feedback)
		OPA_CTLR1_OPA_EN1 | // Enable Op-Amp
		//OPA_CTLR1_OPA_HS1 | // High speed mode.
		//1<<18 |
		0;


## TIA from ground

NSEL = PA1 = D+
PSEL = GND = PA2
PC0 = D- = 3.3V

Just lame.

# TIA from VCC


NSEL = PA1 = D-
PSEL = 3.3v = PA2
PC0 = D+ = 0V


	OPA->CTLR1 =
		//OPA_CTLR1_VBEN | // VBEN to pull + to VDD vs VBEN = 0 -> Directly connect + to that pin.
		//OPA_CTLR1_VBSEL |
		0 | // Connect PSEL1 to PA2
		0 | // Disconnect PSEL1.
		(0b000<<8) | // Connect NSEL1 to PA1
		(0b0<<4) | // PSEL = PD7
		//(0b011<<8) | // Disconnect NSEL1
		//OPA_CTLR1_PGADIF | // Connect PGADIF to PA4 (now VCC) vs GND (0) (If commented)
		OPA_CTLR1_FB_EN1 | // FB_EN1 = 1 (Enable feedback)
		OPA_CTLR1_OPA_EN1 | // Enable Op-Amp
		//OPA_CTLR1_OPA_HS1 | // High speed mode.
		//1<<18 |
		0;

## Another TIA from ground

PC0 = D+ = 0V
NSEL = PA1 = D-
PSEL = PA2 = 0V Driven.
Use OPA to apply slight positive bias.

	OPA->CTLR1 =
		OPA_CTLR1_VBEN | // VBEN to pull + to VDD vs VBEN = 0 -> Directly connect + to that pin.
		OPA_CTLR1_VBSEL |
		0 | // Connect PSEL1 to PA2
		0 | // Disconnect PSEL1.
		(0b000<<8) | // Connect NSEL1 to PA1
		(0b0<<4) | // PSEL = PA2
		//(0b011<<8) | // Disconnect NSEL1
		//OPA_CTLR1_PGADIF | // Connect PGADIF to PA4 (now VCC) vs GND (0) (If commented)
		OPA_CTLR1_FB_EN1 | // FB_EN1 = 1 (Enable feedback)
		OPA_CTLR1_OPA_EN1 | // Enable Op-Amp
		//OPA_CTLR1_OPA_HS1 | // High speed mode.
		//1<<18 |
		0;


Works best out of any of them.

works really good.



