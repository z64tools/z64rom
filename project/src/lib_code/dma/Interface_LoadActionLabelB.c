#include <uLib.h>

/*
   z64ram = 0x8008708C
   z64rom = 0xAFE22C
 */

#ifndef DO_ACTION_TEX_WIDTH
#define DO_ACTION_TEX_WIDTH  48
#define DO_ACTION_TEX_HEIGHT 16
#define DO_ACTION_TEX_SIZE   ((DO_ACTION_TEX_WIDTH * DO_ACTION_TEX_HEIGHT) / 2)         // (sizeof(gCheckDoActionENGTex))
#endif

void Interface_LoadActionLabelB(GlobalContext* globalCtx, u16 action) {
	InterfaceContext* interfaceCtx = &globalCtx->interfaceCtx;
	
#if 0
	if (gSaveContext.language != LANGUAGE_ENG) {
		action += DO_ACTION_MAX;
	}
	
	if (gSaveContext.language == LANGUAGE_FRA) {
		action += DO_ACTION_MAX;
	}
#endif
	
	interfaceCtx->unk_1FC = action;
	
	osCreateMesgQueue(&interfaceCtx->loadQueue, &interfaceCtx->loadMsg, OS_MESG_BLOCK);
	DmaMgr_SendRequest2(
		&interfaceCtx->dmaRequest_160,
		(u32)interfaceCtx->doActionSegment + DO_ACTION_TEX_SIZE,
		gDmaDataTable[17].vromStart + (action * DO_ACTION_TEX_SIZE),
		DO_ACTION_TEX_SIZE,
		0,
		&interfaceCtx->loadQueue,
		NULL,
		NULL,
		0
	);
	osRecvMesg(&interfaceCtx->loadQueue, NULL, OS_MESG_BLOCK);
	
	interfaceCtx->unk_1FA = 1;
}