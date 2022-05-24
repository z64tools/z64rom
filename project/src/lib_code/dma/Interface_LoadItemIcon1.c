#include <uLib.h>

/*
   z64ram = 0x80084A6C
   z64rom = 0xAFBC0C
 */

void Interface_LoadItemIcon1(GlobalContext* globalCtx, u16 button) {
	InterfaceContext* interfaceCtx = &globalCtx->interfaceCtx;
	
	osCreateMesgQueue(&interfaceCtx->loadQueue, &interfaceCtx->loadMsg, OS_MESG_BLOCK);
	DmaMgr_SendRequest2(
		&interfaceCtx->dmaRequest_160,
		(u32)interfaceCtx->iconItemSegment + button * 0x1000,
		gDmaDataTable[7].vromStart + (gSaveContext.equips.buttonItems[button] * 0x1000),
		0x1000,
		0,
		&interfaceCtx->loadQueue,
		NULL,
		NULL,
		0
	);
	osRecvMesg(&interfaceCtx->loadQueue, NULL, OS_MESG_BLOCK);
}