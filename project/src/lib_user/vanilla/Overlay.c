#include <uLib.h>
#include "vt.h"
#include "code/z_actor.h"
#include "code/z_effect_soft_sprite.h"

Asm_VanillaHook(func_800304DC);
void func_800304DC(PlayState* playState, ActorContext* actorCtx, ActorEntry* actorEntry) {
    ActorOverlay* overlayEntry;
    SavedSceneFlags* savedSceneFlags;
    s32 i;
    
    savedSceneFlags = &gSaveContext.sceneFlags[playState->sceneId];
    
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

Asm_VanillaHook(Actor_Spawn);
Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* playState, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {
    Actor* actor;
    ActorInit* actorInit;
    s32 objBankIndex;
    ActorOverlay* overlayEntry;
    u32 temp;
    char* name;
    u32 overlaySize;
    
    osLibPrintf("Spawn Actor: 0x%04X::%04X", (u16)actorId, (u16)params);
    
    overlayEntry = &gActorOverlayTable[actorId];
    Assert(actorId < EXT_ACTOR_MAX);
    
    name = overlayEntry->name != NULL ? overlayEntry->name : "";
    overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
    
    if (HREG(20) != 0) {
        // "Actor class addition [%d:%s]"
        osSyncPrintf("アクタークラス追加 [%d:%s]\n", actorId, name);
    }
    
    if (actorCtx->total > ACTOR_NUMBER_MAX) {
        // "Ａｃｔｏｒ set number exceeded"
        osSyncPrintf(VT_COL(YELLOW, BLACK) "Ａｃｔｏｒセット数オーバー\n" VT_RST);
        
        return NULL;
    }
    
    if (overlayEntry->vramStart == 0) {
        if (HREG(20) != 0) {
            osSyncPrintf("オーバーレイではありません\n"); // "Not an overlay"
        }
        
        actorInit = overlayEntry->initInfo;
    } else {
        if (overlayEntry->loadedRamAddr != NULL) {
            if (HREG(20) != 0) {
                osSyncPrintf("既にロードされています\n"); // "Already loaded"
            }
        } else {
            if (overlayEntry->allocType & ACTOROVL_ALLOC_ABSOLUTE) {
                ASSERT(overlaySize <= ACTOROVL_ABSOLUTE_SPACE_SIZE, "actor_segsize <= AM_FIELD_SIZE", "../z_actor.c",
                       6934);

                if (actorCtx->absoluteSpace == NULL) {
                    // "AMF: absolute magic field"
                    actorCtx->absoluteSpace =
                        ZeldaArena_MallocRDebug(ACTOROVL_ABSOLUTE_SPACE_SIZE, "AMF:絶対魔法領域", 0);
                    if (HREG(20) != 0) {
                        // "Absolute magic field reservation - %d bytes reserved"
                        osSyncPrintf("絶対魔法領域確保 %d バイト確保\n", ACTOROVL_ABSOLUTE_SPACE_SIZE);
                    }
                }

                overlayEntry->loadedRamAddr = actorCtx->absoluteSpace;
            } else if (overlayEntry->allocType & ACTOROVL_ALLOC_PERSISTENT) {
                overlayEntry->loadedRamAddr = ZeldaArena_MallocRDebug(overlaySize, name, 0);
            } else {
                overlayEntry->loadedRamAddr = ZeldaArena_MallocDebug(overlaySize, name, 0);
            }
            
            if (overlayEntry->loadedRamAddr == NULL) {
                // "Cannot reserve actor program memory"
                osSyncPrintf(VT_COL(RED, WHITE) "Ａｃｔｏｒプログラムメモリが確保できません\n" VT_RST);
                
                return NULL;
            }
            
            Overlay_Load(
                overlayEntry->vromStart,
                overlayEntry->vromEnd,
                overlayEntry->vramStart,
                overlayEntry->vramEnd,
                overlayEntry->loadedRamAddr
            );
            
            osSyncPrintf(VT_FGCOL(GREEN));
            osSyncPrintf(
                "OVL(a):Seg:%08x-%08x Ram:%08x-%08x Off:%08x %s\n",
                overlayEntry->vramStart,
                overlayEntry->vramEnd,
                overlayEntry->loadedRamAddr,
                (u32)overlayEntry->loadedRamAddr + (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart,
                (u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr,
                name
            );
            osSyncPrintf(VT_RST);
            
            overlayEntry->numLoaded = 0;
        }
        
        actorInit = (void*)(u32)((overlayEntry->initInfo != NULL)
                     ? (void*)((u32)overlayEntry->initInfo -
            (s32)((u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr))
                     : NULL);
    }
    
    objBankIndex = Object_GetIndex(&playState->objectCtx, actorInit->objectId);
    
    if (
        (objBankIndex < 0) ||
        ((actorInit->category == ACTORCAT_ENEMY) && Flags_GetClear(playState, playState->roomCtx.curRoom.num))
    ) {
        // "No data bank!! <data bank＝%d> (profilep->bank=%d)"
        osSyncPrintf(
            VT_COL(RED, WHITE) "データバンク無し！！<データバンク＝%d>(profilep->bank=%d)\n" VT_RST,
            objBankIndex,
            actorInit->objectId
        );
        Actor_FreeOverlay(overlayEntry);
        
        return NULL;
    }
    
    actor = ZeldaArena_MallocDebug(actorInit->instanceSize, name, 1);
    
    if (actor == NULL) {
        // "Actor class cannot be reserved! %s <size＝%d bytes>"
        osSyncPrintf(
            VT_COL(RED, WHITE) "Ａｃｔｏｒクラス確保できません！ %s <サイズ＝%dバイト>\n",
            VT_RST,
            name,
            actorInit->instanceSize
        );
        Actor_FreeOverlay(overlayEntry);
        
        return NULL;
    }
    
    ASSERT((u8)overlayEntry->numLoaded < 255, "actor_dlftbl->clients < 255", "../z_actor.c", 7031);
    
    overlayEntry->numLoaded++;
    
    if (HREG(20) != 0) {
        // "Actor client No. %d"
        osSyncPrintf("アクタークライアントは %d 個目です\n", overlayEntry->numLoaded);
    }
    
    Lib_MemSet((u8*)actor, actorInit->instanceSize, 0);
    actor->overlayEntry = overlayEntry;
    actor->id = actorInit->id;
    actor->flags = actorInit->flags;
    
    if (actorInit->id == ACTOR_EN_PART) {
        actor->objBankIndex = rotZ;
        rotZ = 0;
    } else {
        actor->objBankIndex = objBankIndex;
    }
    
    actor->init = actorInit->init;
    actor->destroy = actorInit->destroy;
    actor->update = actorInit->update;
    actor->draw = actorInit->draw;
    actor->room = playState->roomCtx.curRoom.num;
    actor->home.pos.x = posX;
    actor->home.pos.y = posY;
    actor->home.pos.z = posZ;
    actor->home.rot.x = rotX;
    actor->home.rot.y = rotY;
    actor->home.rot.z = rotZ;
    actor->params = params;
    
    Actor_AddToCategory(actorCtx, actor, actorInit->category);
    
    temp = gSegments[6];
    Actor_Init(actor, playState);
    gSegments[6] = temp;
    
    return actor;
}

Asm_VanillaHook(EffectSs_Spawn);
void EffectSs_Spawn(PlayState* playState, s32 type, s32 priority, void* initParams) {
    s32 index;
    u32 overlaySize;
    EffectSsOverlay* overlayEntry;
    EffectSsInit* initInfo;
    
    overlayEntry = &gEffectSsOverlayTable[type];
    
    Assert(type < EXT_EFFECT_MAX);
    
    if (EffectSs_FindSlot(priority, &index) != 0) {
        osLibPrintf(osInfo("Break"));
        osLibPrintf("Could not find free slot!");
        
        return;
    }
    
    sEffectSsInfo.searchStartIndex = index + 1;
    overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
    
    if (overlayEntry->vramStart == NULL) {
        osLibPrintf("Not an overlay");
        initInfo = overlayEntry->initInfo;
    } else {
        if (overlayEntry->loadedRamAddr == NULL) {
            overlayEntry->loadedRamAddr = ZeldaArena_MallocRDebug(overlaySize, __FUNCTION__, __LINE__);
            
            if (overlayEntry->loadedRamAddr == NULL) {
                osLibPrintf(osInfo("Break"));
                osLibPrintf("The memory of [" PRNT_BLUE "%.2f" PRNT_RSET "kB] byte cannot be secured. Therefore, the program cannot be loaded.", BinToKb(overlaySize));
                osLibPrintf("What a dangerous situation! Naturally, effects will not produced either.");
                
                return;
            }
            
            Overlay_Load(
                overlayEntry->vromStart,
                overlayEntry->vromEnd,
                overlayEntry->vramStart,
                overlayEntry->vramEnd,
                overlayEntry->loadedRamAddr
            );
        }
        
        initInfo = (void*)(u32)((overlayEntry->initInfo != NULL)
                    ? (void*)((u32)overlayEntry->initInfo -
            (s32)((u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr))
                    : NULL);
    }
    
    if (initInfo->init == NULL) {
        // "Effects have already been loaded, but the constructor is NULL so the addition will not occur.
        // Please fix this. (Waste of memory) %08x %d"
        osLibPrintf(osInfo("Break"));
        osLibPrintf("Effects have already been loaded, but the constructor is NULL so the addition will not occur.");
        osLibPrintf("Please fix this. InitInfo: %08X ID: %d", initInfo, type);
        
        return;
    }
    
    // Delete the previous effect in the slot, in case the slot wasn't free
    EffectSs_Delete(&sEffectSsInfo.table[index]);
    
    sEffectSsInfo.table[index].type = type;
    sEffectSsInfo.table[index].priority = priority;
    
    if (initInfo->init(playState, index, &sEffectSsInfo.table[index], initParams) == 0) {
        osLibPrintf(osInfo("Break"));
        osLibPrintf("Construction failed for some reason. The constructor returned an error");
        osLibPrintf("Ceasing effect addition...");
        EffectSs_Reset(&sEffectSsInfo.table[index]);
    }
}

Asm_VanillaHook(Actor_DrawFaroresWindPointer);
void Actor_DrawFaroresWindPointer(PlayState* play) {
    s32 lightRadius = -1;
    s32 params;
    
    params = gSaveContext.respawn[RESPAWN_MODE_TOP].data;
    
    if (params) {
        f32 yOffset = LINK_IS_ADULT ? 80.0f : 60.0f;
        f32 ratio = 1.0f;
        s32 alpha = 255;
        s32 temp = params - 40;
        
        if (temp < 0) {
            gSaveContext.respawn[RESPAWN_MODE_TOP].data = ++params;
            ratio = ABS(params) * 0.025f;
            D_8015BC14 = 60;
            D_8015BC18 = 1.0f;
        } else if (D_8015BC14) {
            D_8015BC14--;
        } else if (D_8015BC18 > 0.0f) {
            static Vec3f effectVel = { 0.0f, -0.05f, 0.0f };
            static Vec3f effectAccel = { 0.0f, -0.025f, 0.0f };
            static Color_RGBA8 effectPrimCol = { 255, 255, 255, 0 };
            static Color_RGBA8 effectEnvCol = { 100, 200, 0, 0 };
            Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_TOP].pos;
            Vec3f* nextPos = &gSaveContext.respawn[RESPAWN_MODE_DOWN].pos;
            f32 prevNum = D_8015BC18;
            Vec3f dist;
            f32 diff = Math_Vec3f_DistXYZAndStoreDiff(nextPos, curPos, &dist);
            Vec3f effectPos;
            f32 factor;
            f32 length;
            f32 dx;
            f32 speed;
            
            if (diff < 20.0f) {
                D_8015BC18 = 0.0f;
                Math_Vec3f_Copy(curPos, nextPos);
            } else {
                length = diff * (1.0f / D_8015BC18);
                speed = 20.0f / length;
                speed = CLAMP_MIN(speed, 0.05f);
                Math_StepToF(&D_8015BC18, 0.0f, speed);
                factor = (diff * (D_8015BC18 / prevNum)) / diff;
                curPos->x = nextPos->x + (dist.x * factor);
                curPos->y = nextPos->y + (dist.y * factor);
                curPos->z = nextPos->z + (dist.z * factor);
                length *= 0.5f;
                dx = diff - length;
                yOffset += sqrtf(SQ(length) - SQ(dx)) * 0.2f;
            }
            
            effectPos.x = curPos->x + Rand_CenteredFloat(6.0f);
            effectPos.y = curPos->y + 80.0f + (6.0f * Rand_ZeroOne());
            effectPos.z = curPos->z + Rand_CenteredFloat(6.0f);
            
            EffectSsKiraKira_SpawnDispersed(
                play,
                &effectPos,
                &effectVel,
                &effectAccel,
                &effectPrimCol,
                &effectEnvCol,
                1000,
                16
            );
            
            if (D_8015BC18 == 0.0f) {
                gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
                gSaveContext.respawn[RESPAWN_MODE_TOP].playerParams = 0x06FF;
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = 40;
            }
            
            gSaveContext.respawn[RESPAWN_MODE_TOP].pos = *curPos;
        } else if (temp > 0) {
            Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_TOP].pos;
            f32 nextRatio = 1.0f - temp * 0.1f;
            f32 curRatio = 1.0f - (f32)(temp - 1) * 0.1f;
            Vec3f eye;
            Vec3f dist;
            f32 diff;
            
            if (nextRatio > 0.0f) {
                eye.x = play->view.eye.x;
                eye.y = play->view.eye.y - yOffset;
                eye.z = play->view.eye.z;
                diff = Math_Vec3f_DistXYZAndStoreDiff(&eye, curPos, &dist);
                diff = (diff * (nextRatio / curRatio)) / diff;
                curPos->x = eye.x + (dist.x * diff);
                curPos->y = eye.y + (dist.y * diff);
                curPos->z = eye.z + (dist.z * diff);
                gSaveContext.respawn[RESPAWN_MODE_TOP].pos = *curPos;
            }
            
            alpha = 255 - (temp * 30);
            
            if (alpha < 0) {
                gSaveContext.fw.set = 0;
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = 0;
                alpha = 0;
            } else {
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = ++params;
            }
            
            ratio = 1.0f + ((f32)temp * 0.2); // required to match
        }
        
        lightRadius = 500.0f * ratio;
        
        if (
            play->csCtx.state == CS_STATE_IDLE &&
            gSaveContext.respawn[RESPAWN_MODE_TOP].roomIndex == play->roomCtx.curRoom.num &&
            (
                gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex == gSaveContext.entranceIndex
                ||
                gExitParam.respawn[RESPAWN_MODE_TOP].sceneIndex == gExitParam.exit.sceneIndex
            )
        ) {
            f32 scale = 0.025f * ratio;
            
            POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_25);
            
            Matrix_Translate(
                ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x),
                ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y) + yOffset,
                ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.z),
                MTXMODE_NEW
            );
            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
            Matrix_Push();
            
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 128, 128, 255, 255, 200, alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 100, 200, 0, 255);
            
            Matrix_RotateZ(BINANG_TO_RAD_ALT2((play->gameplayFrames * 1500) & 0xFFFF), MTXMODE_APPLY);
            gSPMatrix(
                POLY_XLU_DISP++,
                Matrix_NewMtx(play->state.gfxCtx, "../z_actor.c", 5458),
                G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
            );
            gSPDisplayList(POLY_XLU_DISP++, gEffFlash1DL);
            
            Matrix_Pop();
            Matrix_RotateZ(BINANG_TO_RAD_ALT2(~((play->gameplayFrames * 1200) & 0xFFFF)), MTXMODE_APPLY);
            
            gSPMatrix(
                POLY_XLU_DISP++,
                Matrix_NewMtx(play->state.gfxCtx, "../z_actor.c", 5463),
                G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
            );
            gSPDisplayList(POLY_XLU_DISP++, gEffFlash1DL);
        }
        
        Lights_PointNoGlowSetInfo(
            &D_8015BC00,
            ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x),
            ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y) + yOffset,
            ((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.z),
            255,
            255,
            255,
            lightRadius
        );
    }
}
