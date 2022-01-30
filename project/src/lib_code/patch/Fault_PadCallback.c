#include <oot_mq_debug/z64hdr.h>

void PadMgr_RequestPadData(PadMgr* padmgr, Input* inputs, s32 mode);
asm ("PadMgr_RequestPadData = 0x800C7E08");

void Fault_PadCallback(Input* input) {
	PadMgr_RequestPadData(&gPadMgr, input, 0);
}