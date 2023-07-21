#include <uLib.h>

static void uLib_DebugMessages(u32 msgID) {
    switch (msgID) {
        case 1: {
            ActorOverlay* overlayEntry;
            u32 i;
            
            osLibPrintf("OverlayTable %d", EXT_ACTOR_MAX);
            osLibPrintf("Start   End     SegStart SegEnd   RamAddr profile  segname\n");
            
            for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < EXT_ACTOR_MAX; i++, overlayEntry++) {
                osSyncPrintf(
                    "%08x %08x %08x %08x %08x %08x %s\n",
                    overlayEntry->vromStart,
                    overlayEntry->vromEnd,
                    overlayEntry->vramStart,
                    overlayEntry->vramEnd,
                    overlayEntry->loadedRamAddr,
                    &overlayEntry->initInfo->id,
                    overlayEntry->name != NULL ? overlayEntry->name : "?"
                );
            }
            break;
        }
        
        case 2: {
            ActorOverlay* overlayEntry;
            u32 overlaySize;
            s32 i;
            
            FaultDrawer_SetCharPad(-2, 0);
            
            FaultDrawer_Printf("actor_dlftbls %u\n", gMaxActorId);
            FaultDrawer_Printf("No. RamStart- RamEnd cn  Name\n");
            
            for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < gMaxActorId; i++, overlayEntry++) {
                overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
                if (overlayEntry->loadedRamAddr != NULL) {
                    FaultDrawer_Printf(
                        "%3d %08x-%08x %3d %s\n",
                        i,
                        overlayEntry->loadedRamAddr,
                        (u32)overlayEntry->loadedRamAddr + overlaySize,
                        overlayEntry->numLoaded,
                        overlayEntry->name != NULL ? overlayEntry->name : ""
                    );
                }
            }
        }
    }
}

Asm_VanillaHook(ActorOverlayTable_FaultPrint);
void ActorOverlayTable_FaultPrint(void* arg0, void* arg1) {
    uLib_DebugMessages(2);
}

Asm_VanillaHook(ActorOverlayTable_LogPrint);
void ActorOverlayTable_LogPrint(void) {
    uLib_DebugMessages(1);
}

Asm_VanillaHook(func_800981B8);
void func_800981B8(ObjectContext* objectCtx) {
    s32 i;
    s32 id;
    u32 size;
    
    for (i = 0; i < objectCtx->num; i++) {
        id = objectCtx->status[i].id;
        size = gObjectTable[id].vromEnd - gObjectTable[id].vromStart;
        DmaMgr_SendRequest1(
            objectCtx->status[i].segment,
            gObjectTable[id].vromStart,
            size,
            NULL,
            0
        );
    }
}

Asm_VanillaHook(func_800982FC);
void* func_800982FC(ObjectContext* objectCtx, s32 bankIndex, s16 objectId) {
    ObjectStatus* status = &objectCtx->status[bankIndex];
    RomFile* objectFile = &gObjectTable[objectId];
    u32 size;
    void* nextPtr;
    
    status->id = -objectId;
    status->dmaRequest.vromAddr = 0;
    size = objectFile->vromEnd - objectFile->vromStart;
    nextPtr = (void*)ALIGN16((s32)status->segment + size);
    
    return nextPtr;
}

Asm_VanillaHook(Object_Spawn);
s32 Object_Spawn(ObjectContext* objectCtx, s16 objectId) {
    u32 size;
    
    osLibPrintf("Spawn Object: 0x%04X", (u16)objectId);
    objectCtx->status[objectCtx->num].id = objectId;
    size = gObjectTable[objectId].vromEnd - gObjectTable[objectId].vromStart;
    
    DmaMgr_SendRequest1(
        objectCtx->status[objectCtx->num].segment,
        gObjectTable[objectId].vromStart,
        size,
        __FUNCTION__,
        __LINE__
    );
    
    if (objectCtx->num < OBJECT_EXCHANGE_BANK_MAX - 1)
        objectCtx->status[objectCtx->num + 1].segment =
            (void*)ALIGN16((s32)objectCtx->status[objectCtx->num].segment + size);
    
    objectCtx->num++;
    objectCtx->unk_09 = objectCtx->num;
    
    return objectCtx->num - 1;
}

Asm_VanillaHook(Object_UpdateBank);
void Object_UpdateBank(ObjectContext* objectCtx) {
    s32 i;
    ObjectStatus* status = &objectCtx->status[0];
    RomFile* objectFile;
    u32 size;
    
    for (i = 0; i < objectCtx->num; i++) {
        if (status->id < 0) {
            if (status->dmaRequest.vromAddr == 0) {
                osCreateMesgQueue(&status->loadQueue, &status->loadMsg, 1);
                objectFile = &gObjectTable[-status->id];
                size = objectFile->vromEnd - objectFile->vromStart;
                DmaMgr_SendRequest2(
                    &status->dmaRequest,
                    (u32)status->segment,
                    objectFile->vromStart,
                    size,
                    0,
                    &status->loadQueue,
                    NULL,
                    NULL,
                    0
                );
            } else if (osRecvMesg(&status->loadQueue, NULL, OS_MESG_NOBLOCK) == 0) {
                status->id = -status->id;
            }
        }
        status++;
    }
}

Asm_VanillaHook(func_80091738);
u32 func_80091738(PlayState* playState, u8* segment, SkelAnime* skelAnime) {
    s16 linkObjectId = gLinkObjectIds[(void)0, gSaveContext.linkAge];
    u32 size;
    void* ptr;
    
    size = gObjectTable[OBJECT_GAMEPLAY_KEEP].vromEnd - gObjectTable[OBJECT_GAMEPLAY_KEEP].vromStart;
    ptr = segment + PAUSE_EQUIP_BUFFER_SIZE;
    DmaMgr_SendRequest1(ptr, gObjectTable[OBJECT_GAMEPLAY_KEEP].vromStart, size, NULL, 0);
    
    size = gObjectTable[linkObjectId].vromEnd - gObjectTable[linkObjectId].vromStart;
    ptr = segment + PAUSE_EQUIP_BUFFER_SIZE + PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE;
    DmaMgr_SendRequest1(ptr, gObjectTable[linkObjectId].vromStart, size, NULL, 0);
    
    ptr = (void*)ALIGN16((u32)ptr + size);
    
    gSegments[4] = VIRTUAL_TO_PHYSICAL(segment + PAUSE_EQUIP_BUFFER_SIZE);
    gSegments[6] = VIRTUAL_TO_PHYSICAL(segment + PAUSE_EQUIP_BUFFER_SIZE + PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE);
    
    SkelAnime_InitLink(
        playState,
        skelAnime,
        gPlayerSkelHeaders[(void)0, gSaveContext.linkAge],
        (void*)0x04003238,
        9,
        ptr,
        ptr,
        PLAYER_LIMB_MAX
    );
    
    return size + PAUSE_EQUIP_BUFFER_SIZE + PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE +
           sizeof(Vec3s[PLAYER_LIMB_BUF_COUNT]);
}

Asm_VanillaHook(func_8002F1C4);
s32 func_8002F1C4(Actor* actor, PlayState* playState, f32 xDist, f32 yDist, u32 exchangeItemId) {
    Player* player = GET_PLAYER(playState);
    s32 distCheck = (yDist < fabsf(actor->yDistToPlayer) || xDist < actor->xzDistToPlayer);
    
    if (
        (player->actor.flags & ACTOR_FLAG_8) || ((exchangeItemId != EXCH_ITEM_NONE) && Player_InCsMode(playState)) ||
        ((player->linearVelocity > 0 || distCheck) && !actor->isTargeted)
    ) {
        return false;
    }
    
    player->targetActor = actor;
    player->targetActorDistance = actor->xzDistToPlayer;
    player->exchangeItemId = exchangeItemId;
    
    return true;
}

Asm_VanillaHook(Player_IsChildWithHylianShield);
s32 Player_IsChildWithHylianShield(Player* this) {
#if Patch_WieldHylianShieldLikeKokiriShield == true
    
    return 0;
#endif
    
    return gSaveContext.linkAge && (this->currentShield == PLAYER_SHIELD_HYLIAN);
}
