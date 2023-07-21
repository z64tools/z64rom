#include <uLib.h>

/*
   z64ram = 0x800CA3A0
   z64rom = 0xB41540
 */

extern u32 sSysCfbFbPtr[2];
extern u32 sSysCfbEnd;
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
    sSysCfbFbPtr[1] = sSysCfbEnd - (screenSize * sizeof(u16));
    sSysCfbFbPtr[0] = sSysCfbFbPtr[1] - (screenSize * sizeof(u16));
}
