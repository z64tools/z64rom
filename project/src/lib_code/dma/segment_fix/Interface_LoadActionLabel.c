#include <oot_mq_debug/z64hdr.h>

#define gAttackDoActionENGTex (void*)0x07000000
#define gCheckDoActionENGTex  (void*)0x07000180

#define DO_ACTION_TEX_WIDTH  48
#define DO_ACTION_TEX_HEIGHT 16
#define DO_ACTION_TEX_SIZE   ((DO_ACTION_TEX_WIDTH * DO_ACTION_TEX_HEIGHT) / 2) // (sizeof(gCheckDoActionENGTex))

void func_80086D5C(s32* buf, u16 size);
asm ("func_80086D5C = 0x80086D5C");

void Interface_LoadActionLabel(InterfaceContext* interfaceCtx, u16 action, s16 loadOffset) {
	static void* sDoActionTextures[] = { gAttackDoActionENGTex, gCheckDoActionENGTex };
	
	if (action >= DO_ACTION_MAX) {
		action = DO_ACTION_NONE;
	}
	
	#if 0 // RIP other languages
		if (gSaveContext.language != LANGUAGE_ENG) {
			action += DO_ACTION_MAX;
		}
		
		if (gSaveContext.language == LANGUAGE_FRA) {
			action += DO_ACTION_MAX;
		}
	#endif
	
	if ((action != DO_ACTION_NONE) && (action != DO_ACTION_MAX + DO_ACTION_NONE) && (action != 2 * DO_ACTION_MAX + DO_ACTION_NONE)) {
		osCreateMesgQueue(&interfaceCtx->loadQueue, &interfaceCtx->loadMsg, OS_MESG_BLOCK);
		DmaMgr_SendRequest2(
			&interfaceCtx->dmaRequest_160,
			(u32)interfaceCtx->doActionSegment + (loadOffset * DO_ACTION_TEX_SIZE),
			gDmaDataTable[17].vromStart + (action * DO_ACTION_TEX_SIZE),
			DO_ACTION_TEX_SIZE,
			0,
			&interfaceCtx->loadQueue,
			NULL,
			NULL,
			0
		);
		osRecvMesg(&interfaceCtx->loadQueue, NULL, OS_MESG_BLOCK);
	} else {
		gSegments[7] = VIRTUAL_TO_PHYSICAL(interfaceCtx->doActionSegment);
		func_80086D5C(SEGMENTED_TO_VIRTUAL(sDoActionTextures[loadOffset]), DO_ACTION_TEX_SIZE / 4);
	}
}