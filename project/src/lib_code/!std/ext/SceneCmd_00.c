#include <uLib.h>

/*
   z64ram = 0x80098508
   z64rom = 0xB0F6A8
 */

void func_80098508(PlayState* playState, SceneCmd* cmd) {
    ActorEntry* linkEntry = playState->linkActorEntry = (ActorEntry*)SEGMENTED_TO_VIRTUAL(cmd->spawnList.data) + playState->setupEntranceList[playState->curSpawn].spawn;
    s16 linkObjectId;
    
    playState->linkAgeOnLoad = gSaveContext.linkAge;
    
    linkObjectId = gLinkObjectIds[gSaveContext.linkAge];
    
    gActorOverlayTable[linkEntry->id].initInfo->objectId = linkObjectId;
    Object_Spawn(&playState->objectCtx, linkObjectId);
}
