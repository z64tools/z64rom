#include <uLib.h>

void* Segment_Scene_GetHeader(void* segment, s32 setupIndex) {
    u32* setupAddr = segment;
    
    if (*setupAddr == 0x18000000 && setupIndex != 0) {
        u32* headers_list = (u32*)*(setupAddr + 1);
        
        for (s32 i = setupIndex - 1; i > -1; i--) {
            u32* addr = (u32*)SEGMENTED_TO_VIRTUAL((u32)(headers_list + i));
            
            if (*addr != 0) {
                setupAddr = (u32*)SEGMENTED_TO_VIRTUAL((u32) * addr);
                break;
            }
        }
    }
    
    return setupAddr;
}

void* Segment_Scene_GetCutscene(void* segment, s32 setupIndex) {
    u32* setupAddr = Segment_Scene_GetHeader(segment, setupIndex);
    
    for (s32 i = 0; i < 25; i++) {
        if (*((u8*)(setupAddr + i * 2)) == 0x14)
            break;
        
        if (*(setupAddr + (i * 2)) == 0x17000000)
            return (u32*)*(setupAddr + (i * 2) + 1);
    }
    
    return 0;
}

CollisionHeader* Segment_Scene_GetCollision(void* segment, s32 setupIndex) {
    u32* headerAddr = Segment_Scene_GetHeader(segment, setupIndex);
    
    for (s32 i = 0; i < 25; i++) {
        if (*((u8*)(headerAddr + i * 2)) == 0x14)
            break;
        
        if (*(headerAddr + (i * 2)) == 0x03000000)
            return (CollisionHeader*)SEGMENTED_TO_VIRTUAL(*(headerAddr + (i * 2) + 1));
    }
    
    return NULL;
}

void Segment_Scene_PlayCutscene(void* segment, s32 setupIndex) {
    Cutscene_SetSegment(Effect_GetPlayState(), Segment_Scene_GetCutscene(segment, setupIndex));
    gSaveContext.cutsceneTrigger = true;
}

static void* Segment_GetPathList(s8 header) {
    SceneCmd* hdr = Segment_Scene_GetHeader(gPlayState.sceneSegment,
            header >= 0 ? header : gSaveContext.sceneLayer);
    
    if (!hdr) return NULL;
    
    for (; hdr->base.code != SCENE_CMD_ID_END; hdr++) {
        if (hdr->base.code == SCENE_CMD_ID_PATH_LIST)
            return SEGMENTED_TO_VIRTUAL(hdr->pathList.data);
    }
    
    return NULL;
}

u8 PathList_GetNum(u8 index, s8 header) {
    Path* path = Segment_GetPathList(header);
    
    if (!path) return 0;
    
    return path[index].count;
}

void* PathList_GetList(u8 index, s8 header) {
    Path* path = Segment_GetPathList(header);
    
    if (!path) return 0;
    
    return SEGMENTED_TO_VIRTUAL(path[index].points);
}
