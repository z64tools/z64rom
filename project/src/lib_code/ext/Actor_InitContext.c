#include <ULib.h>

/*
   z64ram = 0x800304DC
   z64rom = 0xAA767C
 */

void func_800304DC(GlobalContext* globalCtx, ActorContext* actorCtx, ActorEntry* actorEntry) {
	ActorOverlay* overlayEntry;
	SavedSceneFlags* savedSceneFlags;
	s32 i;
	
	savedSceneFlags = &gSaveContext.sceneFlags[globalCtx->sceneNum];
	
	bzero(actorCtx, sizeof(*actorCtx));
	
	ActorOverlayTable_Init();
	Matrix_MtxFCopy(&globalCtx->billboardMtxF, &gMtxFClear);
	Matrix_MtxFCopy(&globalCtx->viewProjectionMtxF, &gMtxFClear);
	
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
	
	func_8002CDE4(globalCtx, &actorCtx->titleCtx);
	
	actorCtx->absoluteSpace = NULL;
	
	Actor_SpawnEntry(actorCtx, actorEntry, globalCtx);
	func_8002C0C0(&actorCtx->targetCtx, actorCtx->actorLists[ACTORCAT_PLAYER].head, globalCtx);
	func_8002FA60(globalCtx);
}
