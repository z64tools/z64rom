#include <oot_mq_debug/z64hdr.h>

void Sample_LoadTitleStatic(SampleContext* this) {
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