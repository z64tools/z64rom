#include <uLib.h>

/*
   z64ram = 0x80084B30
   z64rom = 0xAFBCD0
 */

void Interface_LoadItemIcon2(PlayState* playState, u16 button) {
	osCreateMesgQueue(&playState->interfaceCtx.loadQueue, &playState->interfaceCtx.loadMsg, OS_MESG_BLOCK);
	DmaMgr_SendRequest2(
		&playState->interfaceCtx.dmaRequest_180,
		(u32)playState->interfaceCtx.iconItemSegment + button * 0x1000,
		gDmaDataTable[7].vromStart + (gSaveContext.equips.buttonItems[button] * 0x1000),
		0x1000,
		0,
		&playState->interfaceCtx.loadQueue,
		NULL,
		NULL,
		0
	);
	osRecvMesg(&playState->interfaceCtx.loadQueue, NULL, OS_MESG_BLOCK);
}