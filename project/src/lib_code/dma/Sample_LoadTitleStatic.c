#include <ULib.h>

/*
   z64ram = 0x80097904
   z64rom = 0xB0EAA4
 */

void Sample_LoadTitleStatic(SampleContext* this) {
	u32 size = gExtDmaTable[939].vromEnd - gExtDmaTable[939].vromStart;
	
	this->staticSegment = GameState_Alloc(&this->state, size, NULL, 0);
	DmaMgr_SendRequest1(
		this->staticSegment,
		gExtDmaTable[939].vromStart,
		size,
		NULL,
		0
	);
}