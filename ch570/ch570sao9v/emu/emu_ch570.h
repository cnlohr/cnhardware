#ifndef _CORE_H
#define _CORE_H

#include <stdint.h>

// For functions in this code.
uint32_t GetSTK(void);

//==============================================================================
// mini-rv32ima augmentations.
//==============================================================================

static inline void* ch570threadFn(void* v);
static inline uint32_t MINIRV32_LOAD4s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline uint16_t MINIRV32_LOAD2s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline uint8_t MINIRV32_LOAD1s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline int16_t MINIRV32_LOAD2_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline int8_t MINIRV32_LOAD1_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap);

#define RAM_SIZE   0x3000
#define FLASH_SIZE 0x40000

extern uint32_t R32_PA_PU;
extern uint32_t R32_PA_PD_DRV;
extern uint32_t R32_PA_DIR;
extern uint32_t R32_PA_OUT;
extern uint32_t R32_PFIC_IENR1;
extern volatile int ch570runMode;
extern uint8_t ch570flash[FLASH_SIZE];
extern uint8_t ch570ram[RAM_SIZE];
extern struct MiniRV32IMAState ch570state;
extern volatile uint32_t pressures[4];

void Handle_R8_TMR_CTRL_MOD( uint8_t regset );
void Handle_R8_SPI_BUFFER( uint8_t regset );


uint32_t debugpc; // Debug PC

static int CHPLoad(uint32_t address, uint32_t* regret, int size);
static int CHPStore(uint32_t address, uint32_t regret, int size);

#define MINI_RV32_RAM_SIZE		(0x20000000 + RAM_SIZE)
#define MINIRV32_RAM_IMAGE_OFFSET 0x00000000
#define MINIRV32WARN(x...)		fprintf(stderr, x)
#define MINIRV32_MMIO_RANGE(n)	((0x40000000 <= (n) && (n) < 0x50000000)) || (0xe0000000 <= (n) && (n) < 0xe000ffff)

// These should not be accessable.
#define MINIRV32_HANDLE_MEM_STORE_CONTROL(addy, val) CHPStore(addy, val, 4);
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL(addy, val)  CHPLoad(addy, &val, 4);

#define RAMOFS 0x20000000

#define MINIRV32_CUSTOM_MEMORY_BUS

#define MINIRV32_STORE4(ofs, val)						  \
	if (ofs < FLASH_SIZE - 3)							  \
	{													  \
		*(uint32_t*)(ch570flash + ofs) = val;		   \
	}													  \
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 3) \
	{													  \
		*(uint32_t*)(ch570ram + ofs - RAMOFS) = val;	\
	}													  \
	else												   \
	{													  \
		if (CHPStore(ofs, val, 4))						 \
		{												  \
			trap = (7 + 1);								\
			rval = ofs;									\
		}												  \
	}
#define MINIRV32_STORE2(ofs, val)						  \
	if (ofs < FLASH_SIZE - 1)							  \
	{													  \
		*(uint16_t*)(ch570flash + ofs) = val;		   \
	}													  \
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1) \
	{													  \
		*(uint16_t*)(ch570ram + ofs - RAMOFS) = val;	\
	}													  \
	else												   \
	{													  \
		if (CHPStore(ofs, val, 2))						 \
		{												  \
			trap = (7 + 1);								\
			rval = ofs;									\
		}												  \
	}
#define MINIRV32_STORE1(ofs, val)						  \
   { \
	if (ofs < FLASH_SIZE - 0)							  \
	{													  \
		*(uint8_t*)(ch570flash + ofs) = val;			\
	}													  \
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0) \
	{													  \
		*(uint8_t*)(ch570ram + ofs - RAMOFS) = val;	 \
	}													  \
	else												   \
	{													  \
		if (CHPStore(ofs, val, 1))						 \
		{												  \
			trap = (7 + 1);								\
			rval = ofs;									\
		}												  \
	} }
static inline uint32_t MINIRV32_LOAD4s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
	uint32_t tmp = 0;
	if (ofs < FLASH_SIZE - 3)
	{
		tmp = *(uint32_t*)(ch570flash + ofs);
	}
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 3)
	{
		tmp = *(uint32_t*)(ch570ram + ofs - RAMOFS);
	}
	else
	{
		if (CHPLoad(ofs, &tmp, 4))
		{
			*trap = (7 + 1);
			*rval = ofs;
		}
	}
	return tmp;
}
static inline uint16_t MINIRV32_LOAD2s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
	uint16_t tmp = 0;
	if (ofs < FLASH_SIZE - 1)
	{
		tmp = *(uint16_t*)(ch570flash + ofs);
	}
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1)
	{
		tmp = *(uint16_t*)(ch570ram + ofs - RAMOFS);
	}
	else
	{
		if (CHPLoad(ofs, (uint32_t*)&tmp, 2))
		{
			*trap = (7 + 1);
			*rval = ofs;
		}
	}
	return tmp;
}
static inline uint8_t MINIRV32_LOAD1s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
	uint8_t tmp = 0;
	if (ofs < FLASH_SIZE - 0)
	{
		tmp = *(uint8_t*)(ch570flash + ofs);
	}
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0)
	{
		tmp = *(uint8_t*)(ch570ram + ofs - RAMOFS);
	}
	else
	{
		if (CHPLoad(ofs, (uint32_t*)&tmp, 1))
		{
			*trap = (7 + 1);
			*rval = ofs;
		}
	}
	return tmp;
}
static inline int16_t MINIRV32_LOAD2_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
	int16_t tmp = 0;
	if (ofs < FLASH_SIZE - 1)
	{
		tmp = *(int16_t*)(ch570flash + ofs);
	}
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1)
	{
		tmp = *(int16_t*)(ch570ram + ofs - RAMOFS);
	}
	else
	{
		if (CHPLoad(ofs, (uint32_t*)&tmp, -2))
		{
			*trap = (7 + 1);
			*rval = ofs;
		}
	}
	return tmp;
}
static inline int8_t MINIRV32_LOAD1_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
	int8_t tmp = 0;
	if (ofs < FLASH_SIZE - 0)
	{
		tmp = *(int8_t*)(ch570flash + ofs);
	}
	else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0)
	{
		tmp = *(int8_t*)(ch570ram + ofs - RAMOFS);
	}
	else
	{
		if (CHPLoad(ofs, (uint32_t*)&tmp, -1))
		{
			*trap = (7 + 1);
			*rval = ofs;
		}
	}
	return tmp;
}

#define MINIRV32_LOAD4(ofs)		MINIRV32_LOAD4s(ofs, &rval, &trap)
#define MINIRV32_LOAD2(ofs)		MINIRV32_LOAD2s(ofs, &rval, &trap)
#define MINIRV32_LOAD1(ofs)		MINIRV32_LOAD1s(ofs, &rval, &trap)
#define MINIRV32_LOAD2_SIGNED(ofs) MINIRV32_LOAD2_SIGNEDs(ofs, &rval, &trap)
#define MINIRV32_LOAD1_SIGNED(ofs) MINIRV32_LOAD1_SIGNEDs(ofs, &rval, &trap)

#define MINIRV32_HANDLE_TRAP_PC									 \
	switch (CSR(mtvec) & 3)										 \
	{															   \
		case 2:													 \
		case 0:													 \
			pc = (CSR(mtvec) - 4);								  \
			break;												  \
		case 1:													 \
			pc = (((CSR(mtvec) & ~3) + 4 * CSR(mcause)) - 4);	   \
			break;												  \
		case 3:													 \
			pc = ((MINIRV32_LOAD4((CSR(mtvec) & ~3) + 4 * 3)) /* - 4  I thought we needed this */); \
			break;												  \
	}

#define MINIRV32_ALIGNMENT 1

#define MINIRV32_IMPLEMENTATION

// https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/#_compressed_instruction_formats about 1/3 the way
// down. https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
// This adds -C extension support to mini-rv32ima.

#define MINIRV32_HANDLE_OTHER_OPCODE																				   \
	default:																										   \
	{																												  \
		int cimm		 = ((((ir >> 2) & 0x1f)) | (((ir >> 12) & 1) << 5));										   \
		uint32_t cimmext = (cimm & 0x20) ? (cimm | 0xffffffc0) : cimm;												 \
		switch (ir & 3)																								\
		{																											  \
			case 0b00:																								 \
			{																										  \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				uint32_t uimm = (((ir >> 5) & 1) << 6) | (((ir >> 6) & 1) << 2) | (((ir >> 10) & 7) << 3);			 \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.addi4spn ADD Imm * 4 + SP (I think this is right?  Maybe)?*/				   \
						cimm = (((ir >> 5) & 1) << 3) | (((ir >> 6) & 1) << 2) | (((ir >> 7) & 0xf) << 6)			  \
							   | (((ir >> 11) & 3) << 4);															  \
						cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;										 \
						rdid	= ((ir >> 2) & 7) + 8;																 \
						/*printf( "c.addi4spn %08x %d / %d @ %08x\n", REG( 2 ), cimmext, rdid, debugpc );*/							\
						rval = REG(2) + cimmext;																	   \
						break;																						 \
					case 0b010: /*c.lw*/																			   \
						/*printf( "L %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );*/						   \
						rval = MINIRV32_LOAD4(REG(((ir >> 7) & 7) + 8) + uimm);										\
						rdid = ((ir >> 2) & 7) + 8;																	\
						break;																						 \
					case 0b110: /*c.sw*/																			   \
						/*printf( "S %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );*/						   \
						MINIRV32_STORE4(REG(((ir >> 7) & 7) + 8) + uimm, REG(((ir >> 2) & 7) + 8));					\
						rdid = 0;																					  \
						break;																						 \
					default:																						   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			}																										  \
			case 0b01:																								 \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.addi*/																			 \
						rval = REG(rdid) + cimmext;																	\
						break;																						 \
					case 0b010: /*c.li*/																			   \
						rval = cimmext;																				\
						break;																						 \
					case 0b011: /*c.lui / ADDI16SP*/																   \
						switch (((ir >> 7) & 0x1f))																	\
						{																							  \
							case 0:																					\
								break;																				 \
							case 2: /*c.addi16sp TODO: Check me.*/													 \
							{																						  \
								cimm = (((ir >> 12) & 1) << 9) | (((ir >> 2) & 1) << 5) | (((ir >> 5) & 1) << 6)	   \
									   | (((ir >> 6) & 1) << 4) | (((ir >> 3) & 3) << 7);							  \
								cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;								 \
								/*printf( "c.addi16sp -> %08x -> %08x\n",  REG(2), cimmext );*/						\
								rval = REG(2) + cimmext;															   \
								break;																				 \
							}																						  \
							default: /*c.lui*/																		 \
								rval = cimm << 12;																	 \
								break;																				 \
						};																							 \
						break;																						 \
					case 0b100: /* MISC-ALU */																		 \
						rdid = (rdid & 7) + 8;																		 \
						switch ((ir >> 10) & 3)																		\
						{																							  \
							case 0: /*c.srli*/																		 \
								rval = REG(rdid) >> cimm;															  \
								break;																				 \
							case 1: /*c.srai*/																		 \
								rval = ((int32_t)REG(rdid)) >> cimm;												   \
								break;																				 \
							case 2: /*c.andi*/																		 \
								rval = REG(rdid) & cimm;															   \
								break;																				 \
							case 3: /*c.other*/																		\
							{																						  \
								uint32_t rs2 = REG((cimm & 7) + 8);													\
								switch (cimm >> 3)																	 \
								{																					  \
									case 0: /* c.sub */																\
										rval = REG(rdid) - (rs2);													  \
										break;																		 \
									case 1: /* c.xor */																\
										rval = REG(rdid) ^ (rs2);													  \
										break;																		 \
									case 2: /* c.or  */																\
										rval = REG(rdid) | (rs2);													  \
										break;																		 \
									case 3: /* c.and */																\
										rval = REG(rdid) & (rs2);													  \
										break;																		 \
									default: /* res   */															   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
										trap = (2 + 1);																\
										break;																		 \
								}																					  \
							}																						  \
						}																							  \
						break;																						 \
					case 0b001: /*c.jal + NOTE: Is c.addiw on RV64*/												   \
					case 0b101: /*c.j*/																				\
					{																								  \
						uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x7) << 1) | (((ir >> 6) & 0x1) << 7) \
										| (((ir >> 7) & 0x1) << 6) | (((ir >> 8) & 0x1) << 10)						 \
										| (((ir >> 9) & 0x3) << 8) | (((ir >> 11) & 0x1) << 4)						 \
										| (((ir >> 12) & 1) << 11);													\
						if (limm & 0x800)																			  \
							limm |= 0xfffff000;																		\
						if ((ir >> 13) == 0b001)																	   \
						{																							  \
							rdid = 1;																				  \
							rval = pc + 4;																			 \
							/*printf( "jal r1 = %08x\n", rval );*/													 \
						}																							  \
						else																						   \
							rdid = 0;																				  \
						pc = pc + limm - 2;																			\
					}																								  \
					break;																							 \
					case 0b110: /*c.beqz*/																			 \
					case 0b111: /*c.bnez*/																			 \
					{																								  \
						rdid		  = 0;																			 \
						uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x3) << 1)							\
										| (((ir >> 10) & 0x3) << 3) | (((ir >> 5) & 0x3) << 6)						 \
										| (((ir >> 12) & 0x1) << 8);												   \
						if (limm & 0x100)																			  \
							limm |= 0xffffff00;																		\
						if ((ir >> 13) == 0b110)																	   \
						{																							  \
							if (REG(((ir >> 7) & 0x7) + 8) == 0)													   \
							{																						  \
								pc = pc + limm - 2;																	\
							}																						  \
						}																							  \
						else																						   \
						{																							  \
							if (REG(((ir >> 7) & 0x7) + 8) != 0)													   \
							{																						  \
								pc = pc + limm - 2;																	\
							}																						  \
						}																							  \
						break;																						 \
					}																								  \
					default:																						   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			case 0b10:																								 \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.slli*/																			 \
						rval = REG(rdid) << cimm;																	  \
						break;																						 \
					case 0b100: /*c.mv / c.add / c.jr (/c.ret) / c.jalr */											 \
						if ((ir >> 12) & 1)																			\
						{																							  \
							if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)												\
							{																						  \
								/*printf( "Adding r %d / %d\n", (ir>>2)&0x1f, rdid );*/								\
								rval = REG((ir >> 2) & 0x1f) + REG(rdid); /* c.add */								  \
							}																						  \
							else if (rdid != 0)																		\
							{																						  \
								rval = pc + 4;																		 \
								pc   = REG(rdid) - 4; /*c.jr*/														 \
								rdid = 1; /* c.jalr */																 \
							}																						  \
							else																					   \
							{																						  \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
								trap = (3 + 1);																		\
								break; /* EBREAK 3 = "Breakpoint" */												   \
							}																						  \
						}																							  \
						else																						   \
						{                                                                                         \
							if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)												\
							{																						  \
								/* c.mv */                                                                     \
								rval = REG((ir >> 2) & 0x1f);														  \
							/*	printf( "c.mv %d %d -> %08x @ %08x\n", rdid, (ir >> 2) & 0x1f, rval,pc);  */      	\
							}																						  \
							else if (rdid != 0)																		\
							{																						  \
								pc   = REG(rdid) - 4;																  \
								rdid = 0; /* c.jr */																   \
							}																						  \
							else																					   \
							{																						  \
								/* illegal opcode */																   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
								trap = (2 + 1);																		\
								break;																				 \
							}																						  \
						}																							  \
						break;																						 \
					case 0b110: /*c.swsp / c.sw(SP)*/																  \
						rdid = 0;																					  \
						/*printf( "C.SWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>7)&3)<<6)  \
						 * + (((ir>>9)&0xf)<<2), REG(((ir>>2)&0x1f)) ); */											 \
						MINIRV32_STORE4(REG(2) + (((ir >> 7) & 3) << 6) + (((ir >> 9) & 0xf) << 2),					\
										REG(((ir >> 2) & 0x1f)));													  \
						break;																						 \
					case 0b010: /*c.lwsp / c.lw(SP)*/																  \
						/*printf( "C.LWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>2)&3)<<6)  \
						 * + (((ir>>4)&0x7)<<2) + (((ir>>12)&1)<<5), REG(((ir>>2)&0x1f)) );*/						  \
						rval = MINIRV32_LOAD4(REG(2) + (((ir >> 2) & 3) << 6) + (((ir >> 4) & 0x7) << 2)			   \
											  + (((ir >> 12) & 1) << 5));											  \
						rdid = ((ir >> 7) & 0x1f);																	 \
						break;																						 \
					default:																						   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			default:																								   \
printf( "Unknown Opcode at %08x\n", debugpc );                                                                     \
				trap = (2 + 1);																						\
				break;																								 \
		}																											  \
	}

#include "mini-rv32ima.h"


//==============================================================================
// Emulator assistance functions.
//==============================================================================

static uint32_t STK_CTLR;
static uint32_t STK_ZERO;
static uint32_t DMDATA[2];

uint32_t GetSTK()
{
	double dt = OGGetAbsoluteTime();
	if (STK_CTLR & 2)
		dt *= 48000000;
	else
		dt *= 6000000;
	return ((uint32_t)dt) - STK_ZERO;
}

static int CHPLoad(uint32_t address, uint32_t* regret, int size)
{
	if (address == 0x1ffff800 || address == 0x1ffff802)
	{
		// Doing weird option byte operations.
		*regret = 0;
		return 0;
	}

	if (size != 4)
	{
		printf("Misaligned system load %08x\n", address);
		return -1;
	}
	if (address == 0xe000f000)
		*regret = STK_CTLR;
	// else if (address == 0xe00000f4 || address == 0xe00000f8) // CH32V003
	else if (address == 0xe0000340 || address == 0xe0000344) // CH570
		*regret = 0; // Tell DMDATA0/1 that we are free to printf.
	else if (address == 0xe000f008)
		*regret = GetSTK();
	else if (address == 0x40022000)
	{
		*regret = 0;
	} // FLASH->ACTLR
	else if (address == 0x40021000)
	{
		*regret = 3 | (1 << 25);
	} // R32_RCC, lie say clocks are fine.
	else if (address == 0x40021004)
	{
		*regret = 0x8;
	} // R32_RCC, lie and say PLL.
	else if (address == 0x40021008)
	{
		*regret = 0;
	} // R32_RCC
	else if (address == 0x4002100c)
	{
		*regret = 0;
	} // R32_RCC
	else if (address == 0x40021010)
	{
		*regret = 0;
	} // R32_RCC
	else if (address == 0x40021014)
	{
		*regret = 0;
	} // R32_RCC
	else if (address == 0x40021018)
	{
		*regret = 0;
	} // R32_RCC
	else if (address == 0x4002101C)
	{
		*regret = 0;
	} // R32_RCC
	else if( address == 0x40004006 )
	{
		// R8_SPI_INT_FLAG
		*regret = 0x40; // RB_SPI_FREE
	}
	else if( address == 0x4000100a )
	{
		*regret = 0x14; // (R8_HFCK_PWR_CTRL) (STUB)
	}
	else if( address == 0x40004000 )
	{
		*regret = 0x00; // (R8_SPI_CTRL_MOD) (STUB)
	}
	else if( address == 0x40001805 )
	{
		*regret = 0x00; // (R32_FLASH_CONTROL) (STUB)
	}
	else if ( address == 0x4000101a )
	{
		*regret = 0x00; // R16_PIN_ALTERNATE_H (STUB)
	}
	else if( address == 0xe000e100 )
	{
		*regret = R32_PFIC_IENR1;
	}
	else if( address == 0x400010b0 )
	{
		*regret = R32_PA_PU;
	}
	else if( address == 0x400010B4 )
	{
		*regret = R32_PA_PD_DRV;
	}
	else if( address == 0x400010A0 )
	{
		*regret = R32_PA_DIR;
	}
	else if( address == 0x4fff0000 )
	{
		*regret = 0xaaaaaaaa;
	}
	else if( address == 0x4fff0004 )
	{
		*regret = pressures[0];
	}
	else if( address == 0x4fff0008 )
	{
		*regret = pressures[1];
	}
	else if( address == 0x4fff000c )
	{
		*regret = pressures[2];
	}
	else if( address == 0x4fff0010 )
	{
		*regret = pressures[3];
	}
	else if (address >= 0x40000000 && address < 0x50000000)
	{
		printf( "Unknown hardware read %08x @ %08x ra: %08x\n", address, debugpc, ch570state.regs[1] ); *regret = 0;
	}
	else
	{
		printf("Load fail at %08x @ %08x, ra: %08x\n", address, debugpc, ch570state.regs[1]);
		return -1;
	}
	return 0;
}

static int CHPStore(uint32_t address, uint32_t regset, int size)
{
	if (address == 0x1ffff800 || address == 0x1ffff802)
	{
		// Doing weird option byte operations.
		return 0;
	}

#if 0
	// Tricky: when doing write (but only in the emulator) it will do size 1 writes.  This is not true for the real
	// hardware.
	if ((address & 0xfffffffc) == 0x40020064 && size == 1)
	{
		// This code does partial writes.
		int ofs				 = address & 3;
		ch570InternalLEDSets = (ch570InternalLEDSets & (~(0xffu << (ofs * 8)))) | (regset << (ofs * 8));
		return 0;
	}
#endif

	if (size != 4)
	{
		printf("Misaligned system store %08x @ %08x\n", address, debugpc);
		return -1;
	}

	if (address == 0xe000f000)
		STK_CTLR = regset;
	//else if (address == 0xe00000f4 || address == 0xe00000f8) // ch32v003
	else if (address == 0xe0000340 || address == 0xe0000344) // ch570
	{
		//if (address == 0xe00000f4) // ch32v003
		if (address == 0xe0000340) // ch570
			DMDATA[0] = regset;
		else
			DMDATA[1] = regset;

		if (DMDATA[0] & 0x80)
		{
			int chars = (DMDATA[0] & 0xf) - 4;
			int i;
			printf("PRINTF FROM ch570: [%08x %08x]\n", DMDATA[0], DMDATA[1]);
			for (i = 0; i < chars; i++)
			{
				printf("%c", ((uint8_t*)DMDATA)[i + 1]);
			}
			DMDATA[0] = 0;
		}
	}
	else if( address == 0x40001805 )
	{
	} // (R32_FLASH_CONTROL) (STUB)
	else if( address == 0x40001807 )
	{
	} // (R32_FLASH_*) (STUB)
	else if (address == 0x40022000)
	{
	} // FLASH->ACTLR
	else if (address == 0x40021000)
	{
	} // R32_RCC_CFGR0
	else if (address == 0x40021004)
	{
	} // R32_RCC_INTR
	else if (address == 0x40021008)
	{
	} // R32_RCC_APB2PRSTR
	else if (address == 0x4002100c)
	{
	} // R32_RCC_APB1PRSTR
	else if (address == 0x40021010)
	{
	} // R32_RCC_AHBPCENR
	else if (address == 0x40021014)
	{
	} // R32_RCC_APB2PCENR
	else if (address == 0x40021018)
	{
	} // R32_RCC_APB1PCENR
	else if (address == 0x4002101C)
	{
	} // DMA1_Channel5->MADDR
#if 0
	else if (address == 0x40020064)
	{
		ch570InternalLEDSets = regset;
	}
#endif
	else if( address == 0x400010b0 )
	{
		R32_PA_PU = regset;
	}
	else if( address == 0x400010B4 )
	{
		R32_PA_PD_DRV = regset;
	}
	else if( address == 0x400010A0 )
	{
		R32_PA_DIR = regset;
	}
	else if( address == 0x400010AC )
	{
		// R32_PA_CLR
		R32_PA_OUT &= ~regset;
	}
	else if( address == 0x400010B8 )
	{
		// R32_PA_SET
		R32_PA_OUT |= regset;
	}
	else if( address == 0xe000e100 )
	{
		R32_PFIC_IENR1 = regset;
	}
	else if( address == 0x40004004 )
	{
		Handle_R8_SPI_BUFFER( regset );
	}
	else if( address == 0x40002400 )
	{
		Handle_R8_TMR_CTRL_MOD( regset );
	}
	else if( address == 0x40004000 )
	{
		// R8_SPI_CTRL_MOD
	}
	else if( address == 0x40004001 )
	{
		// R8_SPI_CTRL_CFG
	}
	else if( address == 0x40004002 )
	{
		// R8_SPI_INTER_EN
	}
	else if( address == 0x40004003 )
	{
		// R8_SPI_CLOCK_DIV
	}
	else if( address == 0x40001040 )
	{
		// R8_SAFE_ACCESS_SIG
	}
	else if( address == 0x4000100A )
	{
		//R8_HFCK_PWR_CTRL 
	}
	else if( address == 0x40001008 )
	{
		//R8_CLK_SYS_CFG 
	}
	else if ( address == 0x4000101a )
	{
		// R16_PIN_ALTERNATE_H (STUB)
	}
	else if( address == 0x4000240c ); // R32_TMR_CNT_END 
	else if( address == 0x40002402 ); // R8_TMR_INTER_EN
	else if (address >= 0x40000000 && address < 0x50000000)
	{
		 printf( "Unknown hardware write %08x = %08x @ %08x; ra: %08x\n", address, regset, debugpc, ch570state.regs[1] );
	}
	else
	{
		printf("Store fail at %08x @ %08x; ra: %08x\n", address, debugpc, ch570state.regs[1] );
		return -1;
	}
	return 0;
}

static void ResetPeripherals()
{
	STK_ZERO += GetSTK();
	STK_CTLR = 0;
}


int ch570WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address)
{
	uint32_t rval = 0, trap = 0;
	ch570runMode = 0;
	int i;
	for (i = 0; i < length; i++)
		MINIRV32_STORE1(address + i, binary[i]);

	return (trap || rval) ? -1 : 0;
}

int ch570ReadMemory(uint8_t* binary, uint32_t length, uint32_t address)
{
	uint32_t rval = 0, trap = 0;
	ch570runMode = 0;
	int i;
	for (i = 0; i < length; i++)
		binary[i] = MINIRV32_LOAD4(address + i);

	return (trap || rval) ? -1 : 0;
}

int ch570GetReg(int regno, uint32_t* value)
{
	// TODO: Make this work with DMSTATUS
	if (regno == 4)
		*value = DMDATA[0];
	if (regno == 5)
		*value = DMDATA[1];
	return 0;
}

int ch570SetReg(int regno, uint32_t regValue)
{
	// TODO: Actually make this work with DMCONTROL
	if (regno == 4)
		DMDATA[0] = regValue;
	if (regno == 5)
		DMDATA[1] = regValue;
	return 0;
}


int ch570Resume()
{
	ResetPeripherals();
	memset(&ch570state, 0, sizeof(ch570state));
	ch570runMode = 1;
	return 0;
}

#endif

