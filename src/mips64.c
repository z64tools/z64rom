#include "mips64.h"
#include <ext_macros.h>

// Make by Tharo
void Mips64_SplitLoad(u32* hiInsn, u32* loInsn, int dstReg, u32 address) {
	int16_t addrLo = address;
	uint16_t addrHi = address >> 16;
	
	dstReg &= 31; // 5 bits
	
	if (addrLo < 0)            // adjust signed immediate
		addrHi++;
	
	*hiInsn = 0x3C000000 |                  (dstReg << 16) | (u16)addrHi; // lui dstReg, addrHi
	*loInsn = 0x24000000 | (dstReg << 21) | (dstReg << 16) | (u16)addrLo; // addiu dstReg, dstReg, addrLo
	SwapBE(*hiInsn);
	SwapBE(*loInsn);
}
