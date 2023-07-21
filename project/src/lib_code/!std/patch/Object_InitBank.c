/*
   z64ram = 0x80097DD8
   z64rom = 0xB0EF78
 */

#include <uLib.h>

void _Object_InitBank(PlayState* playState, ObjectContext* objectCtx) {
    u32 spaceSize;
    s32 i;
    
    spaceSize = 0x200000; // 2.0 MB
    
    objectCtx->num = objectCtx->unk_09 = 0;
    objectCtx->mainKeepIndex = objectCtx->subKeepIndex = 0;
    
    for (i = 0; i < OBJECT_EXCHANGE_BANK_MAX; i++) {
        objectCtx->status[i].id = OBJECT_INVALID;
    }
    
    objectCtx->spaceStart = objectCtx->status[0].segment =
        GameState_Alloc(&playState->state, spaceSize, __FILE__, __LINE__);
    objectCtx->spaceEnd = (void*)((s32)objectCtx->spaceStart + spaceSize);
    
    objectCtx->mainKeepIndex = Object_Spawn(objectCtx, OBJECT_GAMEPLAY_KEEP);
    gSegments[4] = VIRTUAL_TO_PHYSICAL(objectCtx->status[objectCtx->mainKeepIndex].segment);
}
