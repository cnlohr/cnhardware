#ifndef _FONT8x8_BASIC_TRUNROT
#define _FONT8x8_BASIC_TRUNROT
/**
 * 8x8 monochrome bitmap fonts for rendering
 * Author: Daniel Hepper <daniel@hepper.net>
 *
 * License: Public Domain
 *
 * Based on:
 * // Summary: font8x8.h
 * // 8x8 monochrome bitmap fonts for rendering
 * //
 * // Author:
 * //     Marcel Sondaar
 * //     International Business Machines (public domain VGA fonts)
 * //
 * // License:
 * //     Public Domain
 *
 * Fetched from: http://dimensionalrift.homelinux.net/combuster/mos3/?p=viewsource&file=/modules/gfx/font8_8.asm
 **/

// Constant: font8x8_basic
// Contains an 8x8 font map for unicode points U+0000 - U+007F (basic latin)

// Rotated 90 degrees, and removed 0-31.

static unsigned char font8x8_basic_trunrot[96][8] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, // U+20  
	{ 0x00, 0x00, 0x06, 0x5f, 0x5f, 0x06, 0x00, 0x00, }, // U+21 !
	{ 0x00, 0x03, 0x03, 0x00, 0x03, 0x03, 0x00, 0x00, }, // U+22 "
	{ 0x14, 0x7f, 0x7f, 0x14, 0x7f, 0x7f, 0x14, 0x00, }, // U+23 #
	{ 0x24, 0x2e, 0x6b, 0x6b, 0x3a, 0x12, 0x00, 0x00, }, // U+24 $
	{ 0x46, 0x66, 0x30, 0x18, 0x0c, 0x66, 0x62, 0x00, }, // U+25 %
	{ 0x30, 0x7a, 0x4f, 0x5d, 0x37, 0x7a, 0x48, 0x00, }, // U+26 &
	{ 0x04, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, }, // U+27 '
	{ 0x00, 0x1c, 0x3e, 0x63, 0x41, 0x00, 0x00, 0x00, }, // U+28 (
	{ 0x00, 0x41, 0x63, 0x3e, 0x1c, 0x00, 0x00, 0x00, }, // U+29 )
	{ 0x08, 0x2a, 0x3e, 0x1c, 0x1c, 0x3e, 0x2a, 0x08, }, // U+2a *
	{ 0x08, 0x08, 0x3e, 0x3e, 0x08, 0x08, 0x00, 0x00, }, // U+2b +
	{ 0x00, 0x80, 0xe0, 0x60, 0x00, 0x00, 0x00, 0x00, }, // U+2c ,
	{ 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, }, // U+2d -
	{ 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, }, // U+2e .
	{ 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x01, 0x00, }, // U+2f /
	{ 0x3e, 0x7f, 0x71, 0x59, 0x4d, 0x7f, 0x3e, 0x00, }, // U+30 0
	{ 0x40, 0x42, 0x7f, 0x7f, 0x40, 0x40, 0x00, 0x00, }, // U+31 1
	{ 0x62, 0x73, 0x59, 0x49, 0x6f, 0x66, 0x00, 0x00, }, // U+32 2
	{ 0x22, 0x63, 0x49, 0x49, 0x7f, 0x36, 0x00, 0x00, }, // U+33 3
	{ 0x18, 0x1c, 0x16, 0x53, 0x7f, 0x7f, 0x50, 0x00, }, // U+34 4
	{ 0x27, 0x67, 0x45, 0x45, 0x7d, 0x39, 0x00, 0x00, }, // U+35 5
	{ 0x3c, 0x7e, 0x4b, 0x49, 0x79, 0x30, 0x00, 0x00, }, // U+36 6
	{ 0x03, 0x03, 0x71, 0x79, 0x0f, 0x07, 0x00, 0x00, }, // U+37 7
	{ 0x36, 0x7f, 0x49, 0x49, 0x7f, 0x36, 0x00, 0x00, }, // U+38 8
	{ 0x06, 0x4f, 0x49, 0x69, 0x3f, 0x1e, 0x00, 0x00, }, // U+39 9
	{ 0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, }, // U+3a :
	{ 0x00, 0x80, 0xe6, 0x66, 0x00, 0x00, 0x00, 0x00, }, // U+3b ;
	{ 0x08, 0x1c, 0x36, 0x63, 0x41, 0x00, 0x00, 0x00, }, // U+3c <
	{ 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00, }, // U+3d =
	{ 0x00, 0x41, 0x63, 0x36, 0x1c, 0x08, 0x00, 0x00, }, // U+3e >
	{ 0x02, 0x03, 0x51, 0x59, 0x0f, 0x06, 0x00, 0x00, }, // U+3f ?
	{ 0x3e, 0x7f, 0x41, 0x5d, 0x5d, 0x1f, 0x1e, 0x00, }, // U+40 @
	{ 0x7c, 0x7e, 0x13, 0x13, 0x7e, 0x7c, 0x00, 0x00, }, // U+41 A
	{ 0x41, 0x7f, 0x7f, 0x49, 0x49, 0x7f, 0x36, 0x00, }, // U+42 B
	{ 0x1c, 0x3e, 0x63, 0x41, 0x41, 0x63, 0x22, 0x00, }, // U+43 C
	{ 0x41, 0x7f, 0x7f, 0x41, 0x63, 0x3e, 0x1c, 0x00, }, // U+44 D
	{ 0x41, 0x7f, 0x7f, 0x49, 0x5d, 0x41, 0x63, 0x00, }, // U+45 E
	{ 0x41, 0x7f, 0x7f, 0x49, 0x1d, 0x01, 0x03, 0x00, }, // U+46 F
	{ 0x1c, 0x3e, 0x63, 0x41, 0x51, 0x73, 0x72, 0x00, }, // U+47 G
	{ 0x7f, 0x7f, 0x08, 0x08, 0x7f, 0x7f, 0x00, 0x00, }, // U+48 H
	{ 0x00, 0x41, 0x7f, 0x7f, 0x41, 0x00, 0x00, 0x00, }, // U+49 I
	{ 0x30, 0x70, 0x40, 0x41, 0x7f, 0x3f, 0x01, 0x00, }, // U+4a J
	{ 0x41, 0x7f, 0x7f, 0x08, 0x1c, 0x77, 0x63, 0x00, }, // U+4b K
	{ 0x41, 0x7f, 0x7f, 0x41, 0x40, 0x60, 0x70, 0x00, }, // U+4c L
	{ 0x7f, 0x7f, 0x0e, 0x1c, 0x0e, 0x7f, 0x7f, 0x00, }, // U+4d M
	{ 0x7f, 0x7f, 0x06, 0x0c, 0x18, 0x7f, 0x7f, 0x00, }, // U+4e N
	{ 0x1c, 0x3e, 0x63, 0x41, 0x63, 0x3e, 0x1c, 0x00, }, // U+4f O
	{ 0x41, 0x7f, 0x7f, 0x49, 0x09, 0x0f, 0x06, 0x00, }, // U+50 P
	{ 0x1e, 0x3f, 0x21, 0x71, 0x7f, 0x5e, 0x00, 0x00, }, // U+51 Q
	{ 0x41, 0x7f, 0x7f, 0x09, 0x19, 0x7f, 0x66, 0x00, }, // U+52 R
	{ 0x26, 0x6f, 0x4d, 0x59, 0x73, 0x32, 0x00, 0x00, }, // U+53 S
	{ 0x03, 0x41, 0x7f, 0x7f, 0x41, 0x03, 0x00, 0x00, }, // U+54 T
	{ 0x7f, 0x7f, 0x40, 0x40, 0x7f, 0x7f, 0x00, 0x00, }, // U+55 U
	{ 0x1f, 0x3f, 0x60, 0x60, 0x3f, 0x1f, 0x00, 0x00, }, // U+56 V
	{ 0x7f, 0x7f, 0x30, 0x18, 0x30, 0x7f, 0x7f, 0x00, }, // U+57 W
	{ 0x43, 0x67, 0x3c, 0x18, 0x3c, 0x67, 0x43, 0x00, }, // U+58 X
	{ 0x07, 0x4f, 0x78, 0x78, 0x4f, 0x07, 0x00, 0x00, }, // U+59 Y
	{ 0x47, 0x63, 0x71, 0x59, 0x4d, 0x67, 0x73, 0x00, }, // U+5a Z
	{ 0x00, 0x7f, 0x7f, 0x41, 0x41, 0x00, 0x00, 0x00, }, // U+5b [
	{ 0x01, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, }, // U+5c <backslash>
	{ 0x00, 0x41, 0x41, 0x7f, 0x7f, 0x00, 0x00, 0x00, }, // U+5d ]
	{ 0x08, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x08, 0x00, }, // U+5e ^
	{ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, }, // U+5f _
	{ 0x00, 0x00, 0x03, 0x07, 0x04, 0x00, 0x00, 0x00, }, // U+60 `
	{ 0x20, 0x74, 0x54, 0x54, 0x3c, 0x78, 0x40, 0x00, }, // U+61 a
	{ 0x41, 0x7f, 0x3f, 0x48, 0x48, 0x78, 0x30, 0x00, }, // U+62 b
	{ 0x38, 0x7c, 0x44, 0x44, 0x6c, 0x28, 0x00, 0x00, }, // U+63 c
	{ 0x30, 0x78, 0x48, 0x49, 0x3f, 0x7f, 0x40, 0x00, }, // U+64 d
	{ 0x38, 0x7c, 0x54, 0x54, 0x5c, 0x18, 0x00, 0x00, }, // U+65 e
	{ 0x48, 0x7e, 0x7f, 0x49, 0x03, 0x02, 0x00, 0x00, }, // U+66 f
	{ 0x98, 0xbc, 0xa4, 0xa4, 0xf8, 0x7c, 0x04, 0x00, }, // U+67 g
	{ 0x41, 0x7f, 0x7f, 0x08, 0x04, 0x7c, 0x78, 0x00, }, // U+68 h
	{ 0x00, 0x44, 0x7d, 0x7d, 0x40, 0x00, 0x00, 0x00, }, // U+69 i
	{ 0x60, 0xe0, 0x80, 0x80, 0xfd, 0x7d, 0x00, 0x00, }, // U+6a j
	{ 0x41, 0x7f, 0x7f, 0x10, 0x38, 0x6c, 0x44, 0x00, }, // U+6b k
	{ 0x00, 0x41, 0x7f, 0x7f, 0x40, 0x00, 0x00, 0x00, }, // U+6c l
	{ 0x7c, 0x7c, 0x18, 0x38, 0x1c, 0x7c, 0x78, 0x00, }, // U+6d m
	{ 0x7c, 0x7c, 0x04, 0x04, 0x7c, 0x78, 0x00, 0x00, }, // U+6e n
	{ 0x38, 0x7c, 0x44, 0x44, 0x7c, 0x38, 0x00, 0x00, }, // U+6f o
	{ 0x84, 0xfc, 0xf8, 0xa4, 0x24, 0x3c, 0x18, 0x00, }, // U+70 p
	{ 0x18, 0x3c, 0x24, 0xa4, 0xf8, 0xfc, 0x84, 0x00, }, // U+71 q
	{ 0x44, 0x7c, 0x78, 0x4c, 0x04, 0x1c, 0x18, 0x00, }, // U+72 r
	{ 0x48, 0x5c, 0x54, 0x54, 0x74, 0x24, 0x00, 0x00, }, // U+73 s
	{ 0x00, 0x04, 0x3e, 0x7f, 0x44, 0x24, 0x00, 0x00, }, // U+74 t
	{ 0x3c, 0x7c, 0x40, 0x40, 0x3c, 0x7c, 0x40, 0x00, }, // U+75 u
	{ 0x1c, 0x3c, 0x60, 0x60, 0x3c, 0x1c, 0x00, 0x00, }, // U+76 v
	{ 0x3c, 0x7c, 0x70, 0x38, 0x70, 0x7c, 0x3c, 0x00, }, // U+77 w
	{ 0x44, 0x6c, 0x38, 0x10, 0x38, 0x6c, 0x44, 0x00, }, // U+78 x
	{ 0x9c, 0xbc, 0xa0, 0xa0, 0xfc, 0x7c, 0x00, 0x00, }, // U+79 y
	{ 0x4c, 0x64, 0x74, 0x5c, 0x4c, 0x64, 0x00, 0x00, }, // U+7a z
	{ 0x08, 0x08, 0x3e, 0x77, 0x41, 0x41, 0x00, 0x00, }, // U+7b {
	{ 0x00, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00, 0x00, }, // U+7c |
	{ 0x41, 0x41, 0x77, 0x3e, 0x08, 0x08, 0x00, 0x00, }, // U+7d }
	{ 0x02, 0x03, 0x01, 0x03, 0x02, 0x03, 0x01, 0x00, }, // U+7e ~
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, // U+7f 
};


#endif
