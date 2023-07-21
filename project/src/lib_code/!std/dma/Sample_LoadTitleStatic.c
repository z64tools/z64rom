#include <uLib.h>

/*
   z64ram = 0x80097904
   z64rom = 0xB0EAA4
 */

void Sample_LoadTitleStatic(SampleState* this) {
    u32 size = gDmaDataTable[939].vromEnd - gDmaDataTable[939].vromStart;
    
    this->staticSegment = GameState_Alloc(&this->state, size, NULL, 0);
    DmaMgr_SendRequest1(
        this->staticSegment,
        gDmaDataTable[939].vromStart,
        size,
        NULL,
        0
    );
}
