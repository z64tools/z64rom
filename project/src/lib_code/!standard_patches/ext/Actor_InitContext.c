#include <uLib.h>
#include "code/z_actor.h"

/*
   z64ram = 0x800304DC
   z64rom = 0xAA767C
 */

void func_800304DC(PlayState* playState, ActorContext* actorCtx, ActorEntry* actorEntry) {
	ActorOverlay* overlayEntry;
	SavedSceneFlags* savedSceneFlags;
	s32 i;
	
	savedSceneFlags = &gSaveContext.sceneFlags[playState->sceneNum];
	
	memset(actorCtx, 0, sizeof(*actorCtx));
	
	ActorOverlayTable_Init();
	Matrix_MtxFCopy(&playState->billboardMtxF, &gMtxFClear);
	Matrix_MtxFCopy(&playState->viewProjectionMtxF, &gMtxFClear);
	
	overlayEntry = &gActorOverlayTable[0];
	for (i = 0; i < ARRAY_COUNT(gActorOverlayTable); i++) {
		overlayEntry->loadedRamAddr = NULL;
		overlayEntry->numLoaded = 0;
		overlayEntry++;
	}
	
	actorCtx->flags.chest = savedSceneFlags->chest;
	actorCtx->flags.swch = savedSceneFlags->swch;
	actorCtx->flags.clear = savedSceneFlags->clear;
	actorCtx->flags.collect = savedSceneFlags->collect;
	
	TitleCard_Init(playState, &actorCtx->titleCtx);
	
	actorCtx->absoluteSpace = NULL;
	
	Actor_SpawnEntry(actorCtx, actorEntry, playState);
	func_8002C0C0(&actorCtx->targetCtx, actorCtx->actorLists[ACTORCAT_PLAYER].head, playState);
	func_8002FA60(playState);
}
