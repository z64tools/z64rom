#include <oot_mq_debug/z64hdr.h>

extern u32 osMemSize;
extern u32 sSysCfbFbPtr[2];
extern u32 sSysCfbEnd;
asm ("osMemSize = 0x80000318");
asm ("sSysCfbFbPtr = 0x8016a590");
asm ("sSysCfbEnd = 0x8016a598");

void _SysCfb_Init(s32 n64dd) {
	u32 screenSize;
	
	screenSize = 320 * 240;
	sSysCfbEnd = 0x80800000;
	
	if (osMemSize <= 0x400000U) {
		sSysCfbEnd = 0x80400000;
	}
	
	sSysCfbEnd &= ~0x3f;
	sSysCfbFbPtr[0] = sSysCfbEnd - (screenSize * 4);
	sSysCfbFbPtr[1] = sSysCfbEnd - (screenSize * 2);
}