#include <ext_type.h>

typedef enum {
	MIPS_REG_ZERO = 0,
	MIPS_REG_AT,
	
	MIPS_REG_V0,
	MIPS_REG_V1,
	
	MIPS_REG_A0,
	MIPS_REG_A1,
	MIPS_REG_A2,
	MIPS_REG_A3,
	
	MIPS_REG_T0,
	MIPS_REG_T1,
	MIPS_REG_T2,
	MIPS_REG_T3,
	MIPS_REG_T4,
	MIPS_REG_T5,
	MIPS_REG_T6,
	MIPS_REG_T7,
	
	MIPS_REG_S0,
	MIPS_REG_S1,
	MIPS_REG_S2,
	MIPS_REG_S3,
	MIPS_REG_S4,
	MIPS_REG_S5,
	MIPS_REG_S6,
	MIPS_REG_S7,
	
	MIPS_REG_T8,
	MIPS_REG_T9,
	
	MIPS_REG_K0,
	MIPS_REG_K1,
	
	MIPS_REG_GP,
	MIPS_REG_SP,
	MIPS_REG_FP,
	MIPS_REG_RA
} MipsReg;

#define Mips64_Jal(target) ((3 << 26) | (((target) & ~(7 << 29)) >> 2))
#define Mips64_J(target)   ((2 << 26) | (((target) & ~(7 << 29)) >> 2))
#define Mips64_Nop()       0
#define Mips64_Jr(reg)     0b001000 | ((reg & 0x1F) << (15 + 6))

void Mips64_SplitLoad(u32* hiInsn, u32* loInsn, int dstReg, u32 address);