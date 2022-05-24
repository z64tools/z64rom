#include <uLib.h>

/*
   z64ram = 0x80098508
   z64rom = 0xB0F6A8
 */

void func_80098508(GlobalContext* globalCtx, SceneCmd* cmd) {
	ActorEntry* linkEntry = globalCtx->linkActorEntry = (ActorEntry*)SEGMENTED_TO_VIRTUAL(cmd->spawnList.segment) + globalCtx->setupEntranceList[globalCtx->curSpawn].spawn;
	s16 linkObjectId;
	
	globalCtx->linkAgeOnLoad = gSaveContext.linkAge;
	
	linkObjectId = gLinkObjectIds[gSaveContext.linkAge];
	
	gActorOverlayTable[linkEntry->id].initInfo->objectId = linkObjectId;
	Object_Spawn(&globalCtx->objectCtx, linkObjectId);
}