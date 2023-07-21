#include <uLib.h>
#include <code/z_actor.h>
#include <code/z_lights.h>
#include <code/z_room.h>

typedef struct RoomPointLights {
    void*      roomSegment;
    u32        numlights;
    LightNode* light[32];
} RoomPointLights;

RoomPointLights RoomPointLightsA;
RoomPointLights RoomPointLightsB;
s8 prevTransitionTrigger = 0;

static bool NewRoom_UsesPointlight(void* roomSegment) {
    s8* header = Segment_Scene_GetHeader(roomSegment, gSaveContext.sceneLayer);
    
    while (*header != 0x14)
        header += 8;
    
    return header[1];
}

static void NewRoom_BindLight(PlayState* play, Lights* lights, Room* room) {
    if (!NewRoom_UsesPointlight(room->segment)) {
        Lights_BindAll(lights, play->lightCtx.listHead, 0);
        
        return;
    }
    
    LightInfo* sortedList[7] = { NULL };
    s32 lightCount = Lights_SortLightList(play, sortedList);
    
    if (lightCount <= 0)
        return;
    
    for (s32 i = 0; i < lightCount; i++) {
        if (sortedList[i] == NULL)
            break;
        LightInfo* info = sortedList[i];
        LightParams* params = &info->params;
        Light* light;
        
        if (info->type != LIGHT_DIRECTIONAL) {
            Lights_SetPointlight(play, lights, params, false);
        } else if (info->type == LIGHT_DIRECTIONAL) {
            if ((light = Lights_FindSlot(lights)) == NULL)
                return;
            
            light->l.col[0] = light->l.colc[0] = params->dir.color[0];
            light->l.col[1] = light->l.colc[1] = params->dir.color[1];
            light->l.col[2] = light->l.colc[2] = params->dir.color[2];
            light->l.dir[0] = params->dir.x;
            light->l.dir[1] = params->dir.y;
            light->l.dir[2] = params->dir.z;
            light->l.pad1 = 0;
        }
    }
}

static void NewRoom_DestroyExpiredLights(PlayState* play) {
    RoomPointLights* which;
    void* seg;
    int k;
    
    for (k = 0; k < 2; ++k) {
        switch (k) {
            case 0:
                which = &RoomPointLightsA;
                break;
                
            default:
                which = &RoomPointLightsB;
                break;
        }
        if (
            ((seg = which->roomSegment)
            && seg != play->roomCtx.curRoom.segment
            && seg != play->roomCtx.prevRoom.segment)
            || (play->transitionTrigger == 0 && prevTransitionTrigger > 0)
        ) {
            while (which->numlights)
                LightContext_RemoveLight(
                    play
                    ,
                    &play->lightCtx
                    ,
                    which->light[--which->numlights]
                );
            which->roomSegment = 0;
        }
    }
    
    if (!gSaveContext.fw.set)
        D_8015BC10->info->params.point.radius = 0;
    prevTransitionTrigger = play->transitionTrigger;
}

#if 0
static void NewRoom_BillboardCylinder(PlayState* play) {
    u16* cyl = (u16*)(play->billboardMtx + 1);
    
    MemCpy(cyl, play->billboardMtx, sizeof(Mtx));
    
    /* revert up vector to identity */
    cyl[8 / 2] = 0;            /* x */
    cyl[10 / 2] = 1;           /* y */
    cyl[12 / 2] = 0;           /* z */
    cyl[40 / 2] = 0;           /* x */
    cyl[42 / 2] = 0;           /* y */
    cyl[44 / 2] = 0;           /* z */
}
#endif

void NewRoom_Draw(PlayState* play, Room* room, u32 flags) {
    Lights* lightList;
    
    if (!room->segment)
        return;
    
    gSegments[3] = VIRTUAL_TO_PHYSICAL(room->segment);
    NewRoom_DestroyExpiredLights(play);
    // NewRoom_BillboardCylinder(play);
    lightList = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
    NewRoom_BindLight(play, lightList, room);
    
    if (lightList->numLights > 0)
        Lights_Draw(lightList, play->state.gfxCtx);
    
    sRoomDrawHandlers[room->roomShape->base.type](play, room, flags);
}

Asm_VanillaHook(func_8009728C);
s32 func_8009728C(PlayState* play, RoomContext* roomCtx, s32 roomNum) {
    u32 size;
    
    if (roomCtx->status == 0) {
        roomCtx->prevRoom = roomCtx->curRoom;
        roomCtx->curRoom.num = roomNum;
        roomCtx->curRoom.segment = NULL;
        roomCtx->status = 1;
        
        osLibPrintf("%d / %d", roomNum, play->numRooms);
        Assert(roomNum < play->numRooms);
        
        size = play->roomList[roomNum].vromEnd - play->roomList[roomNum].vromStart;
        roomCtx->unk_34 = (void*)ALIGN16((u32)roomCtx->bufPtrs[roomCtx->unk_30] - ((size + 8) * roomCtx->unk_30 + 7));
        
        osCreateMesgQueue(&roomCtx->loadQueue, &roomCtx->loadMsg, 1);
        DmaMgr_SendRequest2(
            &roomCtx->dmaRequest, (u32)roomCtx->unk_34, play->roomList[roomNum].vromStart, size, 0,
            &roomCtx->loadQueue, NULL, "../z_room.c", 1036
        );
        roomCtx->unk_30 ^= 1;
        
        return 1;
    }
    
    return 0;
}
