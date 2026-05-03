#ifndef _PD_INFO_H
#define _PD_INFO_H

#include <stdint.h>

// https://github.com/eeucalyptus/ch32v003-usbpd/blob/main/pd_phy.c
// https://eeucalyptus.net/2023-12-06-usb-pd-1.html

#define KCODE_MASK (0b11111)

#define KCODE_0 (0)
#define KCODE_1 (1)
#define KCODE_2 (2)
#define KCODE_3 (3)
#define KCODE_4 (4)
#define KCODE_5 (5)
#define KCODE_6 (6)
#define KCODE_7 (7)
#define KCODE_8 (8)
#define KCODE_9 (9)
#define KCODE_A (10)
#define KCODE_B (11)
#define KCODE_C (12)
#define KCODE_D (13)
#define KCODE_E (14)
#define KCODE_F (15)
#define KCODE_SYNC1 (16)
#define KCODE_SYNC2 (17)
#define KCODE_RST1 (18)
#define KCODE_RST2 (19)
#define KCODE_EOP (20)
#define KCODE_SYNC3 (21)
#define KCODE_RESERVED (22)

// KCODE LUT to convert 5-bit KCODE to KCODE enum (see Table 5.1, USB PD 3.2)
const uint8_t pd_kcode_lut[] = {
    /*0b00000*/KCODE_RESERVED,
    /*0b00001*/KCODE_RESERVED,
    /*0b00010*/KCODE_RESERVED,
    /*0b00011*/KCODE_RESERVED,
    /*0b00100*/KCODE_RESERVED,
    /*0b00101*/KCODE_RESERVED,
    /*0b00110*/KCODE_SYNC3,
    /*0b00111*/KCODE_RST1,
    /*0b01000*/KCODE_RESERVED,
    /*0b01001*/KCODE_1,
    /*0b01010*/KCODE_4,
    /*0b01011*/KCODE_5,
    /*0b01100*/KCODE_RESERVED,
    /*0b01101*/KCODE_EOP,
    /*0b01110*/KCODE_6,
    /*0b01111*/KCODE_7,
    /*0b10000*/KCODE_RESERVED,
    /*0b10001*/KCODE_SYNC2,
    /*0b10010*/KCODE_8,
    /*0b10011*/KCODE_9,
    /*0b10100*/KCODE_2,
    /*0b10101*/KCODE_3,
    /*0b10110*/KCODE_A,
    /*0b10111*/KCODE_B,
    /*0b11000*/KCODE_SYNC1,
    /*0b11001*/KCODE_RST2,
    /*0b11010*/KCODE_C,
    /*0b11011*/KCODE_D,
    /*0b11100*/KCODE_E,
    /*0b11101*/KCODE_F,
    /*0b11110*/KCODE_0,
    /*0b11111*/KCODE_RESERVED,
};


#endif

