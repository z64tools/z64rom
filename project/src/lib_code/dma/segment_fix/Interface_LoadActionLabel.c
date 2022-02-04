#include <oot_mq_debug/z64hdr.h>

#ifndef DO_ACTION_MAX
	typedef enum {
		LANGUAGE_ENG,
		LANGUAGE_GER,
		LANGUAGE_FRA,
		LANGUAGE_MAX
	} Language;
	
	typedef enum {
		DO_ACTION_ATTACK,
		DO_ACTION_CHECK,
		DO_ACTION_ENTER,
		DO_ACTION_RETURN,
		DO_ACTION_OPEN,
		DO_ACTION_JUMP,
		DO_ACTION_DECIDE,
		DO_ACTION_DIVE,
		DO_ACTION_FASTER,
		DO_ACTION_THROW,
		DO_ACTION_NONE, // in do_action_static, the texture at this position is NAVI, however this value is in practice the "No Action" value
		DO_ACTION_CLIMB,
		DO_ACTION_DROP,
		DO_ACTION_DOWN,
		DO_ACTION_SAVE,
		DO_ACTION_SPEAK,
		DO_ACTION_NEXT,
		DO_ACTION_GRAB,
		DO_ACTION_STOP,
		DO_ACTION_PUTAWAY,
		DO_ACTION_REEL,
		DO_ACTION_1,
		DO_ACTION_2,
		DO_ACTION_3,
		DO_ACTION_4,
		DO_ACTION_5,
		DO_ACTION_6,
		DO_ACTION_7,
		DO_ACTION_8,
		DO_ACTION_MAX
	} DoAction;
	
	#define DO_ACTION_TEX_WIDTH  48
	#define DO_ACTION_TEX_HEIGHT 16
	#define DO_ACTION_TEX_SIZE   ((DO_ACTION_TEX_WIDTH * DO_ACTION_TEX_HEIGHT) / 2) // (sizeof(gCheckDoActionENGTex))
	
	#define gAttackDoActionENGTex (void*)0x07000000
	#define gCheckDoActionENGTex  (void*)0x07000180
#endif

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