#include <oot_mq_debug/z64hdr.h>

/*
   z64ram = 0x80084B30
   z64rom = 0xAFBCD0
 */

void Interface_LoadItemIcon2(GlobalContext* globalCtx, u16 button) {
	osCreateMesgQueue(&globalCtx->interfaceCtx.loadQueue, &globalCtx->interfaceCtx.loadMsg, OS_MESG_BLOCK);
	DmaMgr_SendRequest2(
		&globalCtx->interfaceCtx.dmaRequest_180,
		(u32)globalCtx->interfaceCtx.iconItemSegment + button * 0x1000,
		gDmaDataTable[7].vromStart + (gSaveContext.equips.buttonItems[button] * 0x1000),
		0x1000,
		0,
		&globalCtx->interfaceCtx.loadQueue,
		NULL,
		NULL,
		0
	);
	osRecvMesg(&globalCtx->interfaceCtx.loadQueue, NULL, OS_MESG_BLOCK);
}